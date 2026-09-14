/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ANStrategy.h"
#include "ANMultipliers.h"

void WotlkDungeonANStrategy::InitTriggers(std::vector<TriggerNode*> &triggers)
{
    // 清怪控制链：门厅那两组守望者（6 只，彼此只隔 15 码）在基线 run436 里 3–5 秒内一起进战斗，
    // 四场承伤 424,711、五场只杀掉 2 只。整本小怪是亡灵，唯一有效的控制是牧师束缚亡灵。
    TrashCcPullStrategy::InitTriggers(triggers);

    // Krik'thir the Gatewatcher
    // TODO: Add CC trigger while web wraps are casting?
    // TODO: Bring healer closer than ranged dps to avoid fixates?
    triggers.push_back(new TriggerNode("krik'thir web wrap",
        { NextAction("attack web wrap", ACTION_RAID + 5) }));
    triggers.push_back(new TriggerNode("krik'thir watchers",
        { NextAction("krik'thir priority", ACTION_RAID + 4) }));

    // Hadronox
    // The core AC triggers are very buggy with this boss, but default strat seems to play correctly

    //Anub'arak
    // 穿刺：上游原注释说追踪不到这些尖刺，实测不成立——它是 creature 29184，
    // 生成后 4 秒才落伤害，半径只有 4 码（英雄 7,539 + 击退），走开两步就能躲。
    // 不躲的代价：run445 a3 一场吃了 105,838，约占全队承伤的四分之一。
    // 与躲践踏同优先级：践踏只在 boss 浮出时、穿刺只在下潜期，两者不会同时触发。
    triggers.push_back(new TriggerNode("anub'arak impale",
        { NextAction("dodge impale", 60.0f) }));
    triggers.push_back(new TriggerNode("anub'arak pound",
        { NextAction("dodge pound", 60.0f) }));
    // 西沿护栏与远程保距（2026-09-13 run463–468 实验）暂不接入：六轮里每一轮都让开局更差——
    // 护栏把全队从准备点拉到 x=536 后牧师离 boss 10 码被践踏秒杀（467/1）、保距在开怪前把牧师挪走导致它整场不进战斗（466/1）、
    // 目标点经 mmap 寻路被吸到平台上/下层（463/464）。动作与触发器代码保留，等站位方案单独设计并可测后再启用。
    // triggers.push_back(new TriggerNode("anub'arak rim", { NextAction("anub'arak rim guard", ACTION_MOVE + 6) }));
    // 远程保距重新接入（2026-09-13 run 473）：三个失败原因已分别修掉——mmap 绕路（改直线 spline + vmap 校验）、
    // 开怪前移动（要求 IsInCombat）、把治疗也挪走（只对远程 DPS）。run 472/4 萨满在准备点被第一记践踏 24k 秒杀，就是没保距。
    // 相关性必须压过 CombatStrategy 的 set facing（ACTION_MOVE+7=37）：run 485 盗贼 46 个践踏读条 tick 里 23 个被 set facing 吃掉、
    // 躲踏没执行，牧师/法师保距 43/27 个 tick 里 31/21 个同样被吃掉。第十五轮改 40/38 后 run 486 剩余被抢占的全是 dps assist（50）/
    // drop target（99）：487/3 盗贼贴脸正面、读条 3 个 tick 被"目标死了→drop target→dps assist"吃光，26.7k 秒杀。再抬到躲踏 60、保距 58，
    // 只剩 drop target（一个 tick）和 ACTION_EMERGENCY 在其上。
    triggers.push_back(new TriggerNode("anub'arak ranged too close", { NextAction("anub'arak keep range", 58.0f) }));
    // 毒疗者优先（第十六轮）实验后**停用**：run 488/1 第二次潜地期 DPS 全在打毒疗者（盗贼 23k vs 其他 1k），两只守卫 15 秒 28k 打死坦克，
    // 随后牧师/萨满也死，boss 脱战重置。守卫才是坦克杀手（破甲 + 5–8.5k 一刀），bot 原生目标选择本来就把约三分之一伤害给守卫。
    // 代码保留（触发器/动作仍注册），等有"没有守卫时才盯毒疗者"的设计再接。
    // triggers.push_back(new TriggerNode("anub'arak venomancer focus", { NextAction("anub'arak focus venomancer", 55.0f) }));
    // 践踏读条 3.2 秒里的两件事（2026-09-13 傍晚，成因见记录「30k+ 践踏」一节）：
    // 坦克破甲 >=3 层时开圣佑术（-50%）；治疗先给主坦上盾再接苦修。
    triggers.push_back(new TriggerNode("anub'arak pound tank", { NextAction("divine protection", ACTION_EMERGENCY) }));
    triggers.push_back(new TriggerNode("anub'arak pound healer",
        { NextAction("anub'arak pound shield tank", ACTION_CRITICAL_HEAL + 9),
          NextAction("anub'arak pound heal tank", ACTION_CRITICAL_HEAL + 8) }));
}

void WotlkDungeonANStrategy::InitMultipliers(std::vector<Multiplier*> &multipliers)
{
    multipliers.push_back(new KrikthirMultiplier(botAI));
    multipliers.push_back(new AnubarakMageManaMultiplier(botAI));
}
