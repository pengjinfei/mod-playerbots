/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "VHTriggers.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

bool ErekemTargetTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "erekem");
    if (!boss) { return false; }

    return botAI->IsDps(bot);
}

bool IchoronTargetTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ichoron");
    if (!boss) { return false; }

    return !botAI->IsHeal(bot);
}

bool VoidShiftTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "zuramat the obliterator");
    if (!boss) { return false; }

    return bot->HasAura(SPELL_VOID_SHIFTED) && !botAI->IsHeal(bot);
}

bool ShroudOfDarknessTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "zuramat the obliterator");
    if (!boss) { return false; }

    return boss->HasAura(SPELL_SHROUD_OF_DARKNESS);
}

bool CyanigosaPositioningTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "cyanigosa");
    if (!boss) { return false; }

    // Include healers here for now, otherwise they stand in things
    return !botAI->IsTank(bot) && !botAI->IsRangedDps(bot);
    // return botAI->IsMelee(bot) && !botAI->IsTank(bot);
}

Creature* VioletHoldPortalKeeper(Player* bot)
{
    Creature* best = nullptr;
    for (uint32 entry : {NPC_PORTAL_GUARDIAN, NPC_PORTAL_KEEPER_1, NPC_PORTAL_KEEPER_2})
        if (Creature* keeper = bot->FindNearestCreature(entry, 80.0f))
            if (bot->IsValidAttackTarget(keeper) && (!best || bot->GetDistance(keeper) < bot->GetDistance(best)))
                best = keeper;
    return best;
}

bool PortalKeeperTrigger::IsActive()
{
    return bot->IsInCombat() && botAI->IsDps(bot) && VioletHoldPortalKeeper(bot);
}
