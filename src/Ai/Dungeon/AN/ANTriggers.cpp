/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ANTriggers.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

bool KrikthirWebWrapTrigger::IsActive()
{
    if (!botAI->IsDps(bot)) { return false; }

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_WEB_WRAP)
        {
            return true;
        }
    }

    return false;
}

bool KrikthirWatchersTrigger::IsActive()
{
    if (!botAI->IsDps(bot)) { return false; }

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_KRIKTHIR)
        {
            return true;
        }
    }
    return false;
}

// bool AnubarakImpaleTrigger::IsActive()
// {
//     Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
//     if (!boss) { return false; }
//     GuidVector triggers = AI_VALUE(GuidVector, "possible triggers");
//     for (auto i = triggers.begin(); i != triggers.end(); i++)
//     {
//         Unit* unit = botAI->GetUnit(*i);

//         if (unit)
//         {
//             bot->Yell("TRIGGER="+unit->GetName(), LANG_UNIVERSAL);
//         }
//     }
//     return false;
// }

Unit* FindNearestImpaleSpike(Player* bot, float range)
{
    // 尖刺带 UNIT_FLAG_NOT_SELECTABLE，不会出现在 "possible targets no los" 里，
    // 所以直接按 entry 就近搜，不走选目标那套。
    return bot->FindNearestCreature(NPC_IMPALE_TARGET, range);
}

bool AnubarakImpaleTrigger::IsActive()
{
    return FindNearestImpaleSpike(bot, kImpaleTriggerRadius) != nullptr;
}

bool AnubarakPoundTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss) { return false; }

    return boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(SPELL_POUND);
}

bool AnubarakRimTrigger::IsActive()
{
    if (!bot->IsAlive())
        return false;
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss)
        return false;
    if (bot->GetPositionX() < kArenaSafeMinX)
        return true;
    float const dx = bot->GetPositionX() - kArenaCenterX;
    float const dy = bot->GetPositionY() - kArenaCenterY;
    return dx * dx + dy * dy > kArenaSafeRadius * kArenaSafeRadius;
}

bool AnubarakRangedTooCloseTrigger::IsActive()
{
    // 战斗中才拉开：run 466/1 里开怪前就把牧师挪走，它没进战斗状态，整场留在非战斗引擎里。
    // 只管远程 DPS：治疗靠躲踏（10 码内）保命，别让它为拉开距离浪费治疗 GCD。
    if (!bot->IsAlive() || !bot->IsInCombat() || !botAI->IsRangedDps(bot))
        return false;
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss || !boss->IsAlive() || boss->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))  // 潜地期没有践踏
        return false;
    return bot->GetExactDist(boss) < kRangedKeepDistance + bot->GetObjectSize();
}
