/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORTRIGGERS_H
#define PLAYERBOTS_HORTRIGGERS_H

#include "Trigger.h"

class Player;

enum HoRWaveBosses
{
    NPC_FALRIC = 38112,
    NPC_MARWYN = 38113,
};

// Falric and Marwyn wait in the hall, immune to players, until their own wave (5th and 10th).
enum class HoRWaveBossState
{
    None,     // neither is around (Frostsworn General, the escape)
    Waiting,  // at least one is still waiting for his wave: the fight is spirits only
    Engaged,  // one of them is attackable and fighting
};

HoRWaveBossState GetHoRWaveBossState(Player* bot);

// Heroism/Bloodlust once Falric or Marwyn engages. The generic boost trigger fires on "balance", which
// against a single 5-man boss never drops to its 50% threshold (Falric: 5x80 against 3x82), so on the
// boss itself it never casts.
class HoRWaveBossBoostTrigger : public Trigger
{
public:
    HoRWaveBossBoostTrigger(PlayerbotAI* ai) : Trigger(ai, "hor wave boss boost") {}

    bool IsActive() override;
};

#endif
