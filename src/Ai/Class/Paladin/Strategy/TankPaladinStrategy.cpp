/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "TankPaladinStrategy.h"
#include "Playerbots.h"

class TankPaladinStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    TankPaladinStrategyActionNodeFactory()
    {
        creators["seal of corruption"] = &seal_of_corruption;
        creators["seal of vengeance"] = &seal_of_vengeance;
        creators["seal of command"] = &seal_of_command;
        creators["hand of reckoning"] = &hand_of_reckoning;
        creators["taunt spell"] = &hand_of_reckoning;
    }

private:
    static ActionNode* seal_of_command([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "seal of command",
            /*P*/ {},
            /*A*/ { NextAction("seal of corruption") },
            /*C*/ {}
        );
    }
    static ActionNode* seal_of_corruption([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "seal of corruption",
            /*P*/ {},
            /*A*/ { NextAction("seal of vengeance") },
            /*C*/ {}
        );
    }

    static ActionNode* seal_of_vengeance([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "seal of vengeance",
            /*P*/ {},
            /*A*/ { NextAction("seal of righteousness") },
            /*C*/ {}
        );
    }

    static ActionNode* hand_of_reckoning([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode(
            "hand of reckoning",
            /*P*/ {},
            /*A*/ { NextAction("righteous defense") },
            /*C*/ {}
        );
    }
};

TankPaladinStrategy::TankPaladinStrategy(PlayerbotAI* botAI) : GenericPaladinStrategy(botAI)
{
    actionNodeFactories.Add(new TankPaladinStrategyActionNodeFactory());
}

std::vector<NextAction> TankPaladinStrategy::getDefaultActions()
{
    return {
        NextAction("shield of righteousness", ACTION_DEFAULT + 0.6f),
        NextAction("hammer of the righteous", ACTION_DEFAULT + 0.5f),
        NextAction("judgement of wisdom", ACTION_DEFAULT + 0.4f),
        NextAction("melee", ACTION_DEFAULT)
    };
}

void TankPaladinStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericPaladinStrategy::InitTriggers(triggers);

    triggers.push_back(
        new TriggerNode(
            "seal",
            {
                NextAction("seal of corruption", ACTION_HIGH)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "low mana",
            {
                NextAction("seal of wisdom", ACTION_HIGH + 9)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "light aoe",
            {
                NextAction("avenger's shield", ACTION_HIGH + 5)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "medium aoe",
            {
                // 2026-09-17：奉献相关性从 ACTION_HIGH+7(27) 提到 52。
                //
                // 奉献是防骑唯一的持续 AoE 仇恨来源（8 秒持续 / 8 秒冷却，本该接近 100% 覆盖）。
                // 英雄斯拉德兰 16 场实测：**只放 4.6 次/场，而理论上限是 11.8 次/场（39% 饱和度）**；
                // **相邻两次的间隔中位 12.5 秒**（冷却只有 8 秒 → 每次白等 4.5 秒），
                // 只有 25% 的间隔 ≤10 秒、13 次 >20 秒（最长 41.7 秒），
                // **最后一次释放到战斗结束还剩 32.5 秒**（末段完全断档）。
                //
                // tick 级（2 场，LogInGroupOnly=0）：推入 236 次只执行 8 次（3.4%）。
                // 有奉献推入的 tick 里 96 次被别人抢到，**主抢占者就是 `tank assist`(50)×24**
                // （其余相关性更高的 avoid poison nova(65)/drop target(99) 是对的，不该让）。
                // 结果码里 USELESS 29 + IMPOSSIBLE 中带
                // "Can cast spell failed. Spell not has cooldown." 59 次 —— 都是冷却，**不吃 tick**，
                // 所以抬高相关性不会挤占其它动作（同「神圣恳求常驻」那次的理由）。
                //
                // 52：压过 tank assist(50)，低于 righteous defense on party(55)、
                // avoid poison nova(65)、drop target(99)。
                //
                // 头寸来自哪里：第一刀放开 AoE 之后，**法师用暴风雪/烈焰风暴把小怪仇恨全拉到自己身上**
                // （24 场里法师一个人承受 33.2% 的小怪伤害，是坦克的 1.4 倍）。
                // 第六刀用镜像把这个比例压到 23%，剩下的要靠坦克自己的 AoE 仇恨。
                NextAction("consecration", 52.0f),
                // 2026-09-17：试过在这里加 `NextAction("hammer of the righteous", 51.0f)`，
                // **净负面、已回退**。
                //
                // 靶子指标确实动了：神圣之锤 2.20 → 5.20 次/场（+136%，饱和度 14% → 33%）。
                // 但结果全面变差（run606 vs run611，同配置各 5 场）：
                //   击杀 2/5 → **0/5**；打到 boss 的伤害 269.9k → 242.9k（-10%）；
                //   团灭场 boss 残血 5.0% → **15.0%**；
                //   **奉献 7.20 → 6.40 次/场（-11%）**；
                //   **坦克承受的小怪伤害 34.2k → 25.4k（-26%）**，而小怪总承伤反而 +7.5%
                //   —— 坦克接住的怪更少了。
                //
                // 机制：**神圣之锤和奉献抢同一个 GCD**。两者在同一节点、相关性只差 1，
                // 奉献先被评估；神圣之锤放掉之后占住 GCD，下一 tick 奉献冷却好了却卡在
                // 公共冷却里，于是奉献被推迟。而**奉献是不限目标的地面 AoE、神圣之锤只命中 3 个**
                // —— 用后者换前者是亏的。
                //
                // 教训：**两个都要 GCD 的技能不要挂在同一个高相关性节点上互相推迟；
                // 先保证价值最高的那个满覆盖。** 神圣之锤留在填充档
                // （`getDefaultActions()` 的 `ACTION_DEFAULT + 0.5f`）就好。
                NextAction("avenger's shield", ACTION_HIGH + 6)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "lose aggro",
            {
                NextAction("hand of reckoning", ACTION_HIGH + 7)
            }
        )
    );
    // 防骑的续蓝闭环是「庇护祝福回蓝 + 精神协调 + 神圣恳求常驻（圣光守护 2/2 靠近战刷新）」，
    // 但 GenericPaladinStrategy 把 divine plea 挂在 ACTION_HIGH(20)：实测坦克 3733 个 tick 里
    // 恳求推入队列 381 次、只执行 9 次（2.4%），全被 tank assist(50)/drop target(99)/set facing/
    // hand of reckoning(27) 抢走；恳求不在身上，圣光守护每场 123 次刷新就全刷在空气上。
    // 这里给坦克单独挂一个压过 tank assist 的节点（52，仍低于接小怪 55 / 保距 58 / 躲踏 60），
    // 触发条件也换成「战斗中且恳求不在身上」——上游的 HighManaTrigger 是蓝<65% 才亮，
    // 坦克开局 80% 起步，run 507 两场因此一次都没放（相关性修了、门槛还挡着）。
    // 恳求是 buff，冷却中/已在身上时返回 USELESS 而不吃 tick，所以抬高相关性不会挤占其它动作。
    triggers.push_back(
        new TriggerNode(
            "divine plea uptime",
            {
                NextAction("divine plea", ACTION_HIGH + 32.0f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "party member melee aggro",
            {
                // 必须压过 tank assist(50) 与 tank face(30)：run 499 实测 26.5 的 tick 全被它们吃掉，
                // 一次都没执行。低于 AN 层躲踏/保距（58/60）——躲 25k 践踏比接小怪优先。
                NextAction("righteous defense on party", ACTION_HIGH + 35.0f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "medium health",
            {
                NextAction("holy shield", ACTION_HIGH + 4)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "avenging wrath",
            {
                NextAction("avenging wrath", ACTION_HIGH + 2)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "target critical health",
            {
                NextAction("hammer of wrath", ACTION_CRITICAL_HEAL)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "righteous fury",
            {
                NextAction("righteous fury", ACTION_HIGH + 8)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "medium group heal setting",
            {
                NextAction("divine sacrifice", ACTION_HIGH + 5)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "enough mana",
            {
                NextAction("consecration", ACTION_HIGH + 4)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "not facing target",
            {
                NextAction("set facing", ACTION_NORMAL + 7)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "enemy out of melee",
            {
                NextAction("reach melee", ACTION_HIGH + 1)
            }
        )
    );
}
