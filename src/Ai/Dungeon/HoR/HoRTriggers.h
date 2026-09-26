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

// Lich King escape: the Lich King (36954, unkillable, heals back to 75%) walks after the leader, who runs from ice
// wall to ice wall while the walls' summons are killed. Remorseless Winter (69780) is up for the whole run.
enum HoREscapeIds
{
    NPC_ESCAPE_LICH_KING         = 36954,
    NPC_ESCAPE_LEADER_JAINA      = 36955,
    NPC_ESCAPE_LEADER_SYLVANAS   = 37554,
    NPC_RISEN_WITCH_DOCTOR       = 36941,
    NPC_LUMBERING_ABOMINATION    = 37069,
    NPC_RAGING_GHOUL             = 36940,
    SPELL_REMORSELESS_WINTER     = 69780,
};

// Every 2 s the Lich King zaps (10k) anyone with (x - lkX) + (y - lkY) > 20: the run heads towards -x/-y.
constexpr float HOR_ESCAPE_ZAP_BEHIND = 20.0f;

struct HoREscape
{
    Creature* lichKing = nullptr;
    Creature* leader = nullptr;
};

// Both are set only while the escape runs (the Lich King has Remorseless Winter).
HoREscape GetHoREscape(Player* bot);

// Keep up with the leader and stay clear of the Lich King's back: behind him, or a wall cleared and the leader gone
// on while the bot still stands at the last one.
class HoREscapeKeepUpTrigger : public Trigger
{
public:
    HoREscapeKeepUpTrigger(PlayerbotAI* ai) : Trigger(ai, "hor escape keep up") {}

    bool IsActive() override;
};

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
