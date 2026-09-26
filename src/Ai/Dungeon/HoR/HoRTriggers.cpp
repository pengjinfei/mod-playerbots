/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRTriggers.h"
#include "Playerbots.h"

HoRWaveBossState GetHoRWaveBossState(Player* bot)
{
    HoRWaveBossState state = HoRWaveBossState::None;
    for (uint32 entry : { NPC_FALRIC, NPC_MARWYN })
    {
        Creature* boss = bot->FindNearestCreature(entry, 150.0f);
        if (!boss || !boss->IsAlive())
            continue;
        if (!boss->IsImmuneToPC() && boss->IsInCombat())
            return HoRWaveBossState::Engaged;
        state = HoRWaveBossState::Waiting;
    }
    return state;
}

bool HoRWaveBossBoostTrigger::IsActive()
{
    if (bot->getClass() != CLASS_SHAMAN || !bot->IsInCombat())
        return false;
    if (bot->HasAura(57723) || bot->HasAura(57724))  // Exhaustion / Sated
        return false;
    return GetHoRWaveBossState(bot) == HoRWaveBossState::Engaged;
}
