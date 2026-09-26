/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "FoSTriggers.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

bool MoveFromBronjahmTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "bronjahm");
    if (!boss)
        return false;

    // The fragment spawns where the target stands when the 4 s debuff ends and walks to Bronjahm; the farther out it
    // appears, the longer the group has to kill it (heroic run1125: one fragment reached him and healed 34% -> 61%).
    // Hold the distance for the whole debuff, not only during his cast.
    if (!bot->HasAura(SPELL_CORRUPT_SOUL))
        return false;

    return true;
}

bool SwitchToSoulFragment::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "bronjahm");
    if (!boss)
        return false;

    Unit* corruptedSoul = bot->FindNearestCreature(NPC_CORRUPTED_SOUL_FRAGMENT, 50.0f);
    bool activeSoulExists = corruptedSoul && corruptedSoul->IsAlive();

    if (!activeSoulExists)
        return false;

    return true;
}

bool BronjahmPositionTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "bronjahm");
    if (!boss)
        return false;

    if (bot->HasAura(SPELL_CORRUPT_SOUL))
        return false;

    return true;
}

bool DevourerOfSoulsTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "devourer of souls");
    if (!boss)
        return false;

    return true;
}
