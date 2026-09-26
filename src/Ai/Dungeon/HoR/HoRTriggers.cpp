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

HoREscape GetHoREscape(Player* bot)
{
    HoREscape escape;
    Creature* lichKing = bot->FindNearestCreature(NPC_ESCAPE_LICH_KING, 200.0f);
    if (!lichKing || !lichKing->IsAlive() || !lichKing->HasAura(SPELL_REMORSELESS_WINTER))
        return escape;

    Creature* leader = bot->FindNearestCreature(NPC_ESCAPE_LEADER_JAINA, 250.0f);
    if (!leader)
        leader = bot->FindNearestCreature(NPC_ESCAPE_LEADER_SYLVANAS, 250.0f);
    if (!leader || !leader->IsAlive())
        return escape;

    escape.lichKing = lichKing;
    escape.leader = leader;
    return escape;
}

bool HoREscapeKeepUpTrigger::IsActive()
{
    HoREscape const escape = GetHoREscape(bot);
    if (!escape.lichKing)
        return false;

    float const behind = (bot->GetPositionX() - escape.lichKing->GetPositionX()) +
                         (bot->GetPositionY() - escape.lichKing->GetPositionY());
    if (behind > HOR_ESCAPE_ZAP_BEHIND - 8.0f)
        return true;

    // Summons of the current wall still up nearby: fight them here.
    for (uint32 entry : { NPC_RISEN_WITCH_DOCTOR, NPC_LUMBERING_ABOMINATION, NPC_RAGING_GHOUL })
        if (Creature* add = bot->FindNearestCreature(entry, 30.0f, true))
            if (add->IsInCombat())
                return false;

    return bot->GetExactDist2d(escape.leader) > 15.0f;
}

bool HoRWaveBossBoostTrigger::IsActive()
{
    if (bot->getClass() != CLASS_SHAMAN || !bot->IsInCombat())
        return false;
    if (bot->HasAura(57723) || bot->HasAura(57724))  // Exhaustion / Sated
        return false;
    return GetHoRWaveBossState(bot) == HoRWaveBossState::Engaged;
}
