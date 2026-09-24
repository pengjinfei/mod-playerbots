/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ANMultipliers.h"
#include "ANActions.h"
#include "ANTriggers.h"
#include "Action.h"
#include "ChooseTargetActions.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"

float KrikthirMultiplier::GetValue(Action* action)
{
    if (!botAI->IsDps(bot)) { return 1.0f; }

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    Unit* boss = nullptr;
    Unit* watcher = nullptr;

    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");
    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (!unit) { continue; }

        switch (unit->GetEntry())
        {
            case NPC_KRIKTHIR:
                boss = unit;
                continue;
            case NPC_WATCHER_SILTHIK:
            case NPC_WATCHER_GASHRA:
            case NPC_WATCHER_NARJIL:
            case NPC_WATCHER_SKIRMISHER:
            case NPC_WATCHER_SHADOWCASTER:
            case NPC_WATCHER_WARRIOR:
                watcher = unit;
                continue;
        }
    }

    if (boss && watcher)
    {
        // Do not target swap
        if (dynamic_cast<DpsAssistAction*>(action))
        {
            return 0.0f;
        }

        if (action->getThreatType() == Action::ActionThreatType::Aoe)
        {
            return 0.0f;
        }
    }
    return 1.0f;
}

float HadronoxCrusherMageMultiplier::GetValue(Action* action)
{
    if (!action || bot->getClass() != CLASS_MAGE || !bot->IsInCombat())
        return 1.0f;

    std::string const& name = action->getName();
    bool const aoe = name == "blizzard" || name == "flamestrike" || name == "blast wave" || name == "dragon's breath" ||
                     name == "living bomb on attacker" || name == "living bomb on attackers";
    if (!aoe)
        return 1.0f;

    constexpr uint32 kAnubArCrusher = 28922;
    Creature* crusher = bot->FindNearestCreature(kAnubArCrusher, 60.0f);
    return (crusher && crusher->IsAlive()) ? 0.0f : 1.0f;
}

float AnubarakMageManaMultiplier::GetValue(Action* action)
{
    if (!action || bot->getClass() != CLASS_MAGE)
        return 1.0f;

    // 注意动作实例名是 "living bomb on attacker"（CastDebuffSpellOnAttackerAction 拼的单数），
    // 策略节点/注册名才是 "living bomb on attackers"。run 469 之前按复数匹配一次都没压住：32 次炸弹里 24 次铺在小怪上，约 17k 蓝。
    std::string const& name = action->getName();
    bool const aoe = name == "blizzard" || name == "flamestrike" || name == "living bomb on attacker" ||
                     name == "living bomb on attackers";
    bool const singleBomb = name == "living bomb";
    if (!aoe && !singleBomb)
        return 1.0f;

    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss)
        return 1.0f;

    if (singleBomb)
    {
        // 单体炸弹只留给 boss：run 462 仍每场 22–34 次，跟着当前目标换到哪只小怪就往哪只上，约 720 蓝一个。
        Unit* target = action->GetTarget();
        return (target && target->GetEntry() == kAnubarakEntry) ? 1.0f : 0.0f;
    }

    // 第十九轮：群攻不再一律归零。run 489–492 法师 135 个 AOE tick（≥3 小怪聚在目标 8 码内）里烈焰风暴/暴风雪 0 次，
    // 旧 boss 里同一法师烈焰风暴 10 次。条件放行：boss 潜地（不可选中，只有小怪可打）、目标 8 码内 ≥3 只活着的小怪、法师蓝 >50%。
    // 铺炸弹（living bomb on attacker）仍归零——它是单体 DoT 逐只铺，run 469 之前每场约 17k 蓝。
    if (name == "living bomb on attacker" || name == "living bomb on attackers")
        return 0.0f;
    if (!boss->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return 0.0f;
    if (bot->GetPowerPct(POWER_MANA) <= 50.0f)
        return 0.0f;
    Unit* target = action->GetTarget();
    if (!target)
        return 0.0f;
    uint32 clustered = 0;
    for (ObjectGuid const& guid : botAI->GetAiObjectContext()->GetValue<GuidVector>("attackers")->Get())
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && unit->GetDistance(target) <= 8.0f)
            ++clustered;
    }
    return clustered >= 3 ? 1.0f : 0.0f;
}

float AnubarakHeroismMultiplier::GetValue(Action* action)
{
    if (!action || bot->getClass() != CLASS_SHAMAN)
        return 1.0f;

    std::string const& name = action->getName();
    if (name != "heroism" && name != "bloodlust")
        return 1.0f;

    // 本策略也覆盖克里克塞尔/哈德诺克斯：那两战找不到阿努巴拉克，不干预。
    Creature* boss = bot->FindNearestCreature(kAnubarakEntry, 200.0f);
    if (!boss || !boss->IsAlive())
        return 1.0f;

    // 第三次出土之前一律不许放（判据与 AnubarakHeroismTrigger 完全一致）
    return AnubarakAfterThirdEmerge(botAI, bot) ? 1.0f : 0.0f;
}
