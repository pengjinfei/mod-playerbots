/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HOSTRIGGERS_H
#define PLAYERBOTS_HOSTRIGGERS_H

#include "DungeonStrategyUtils.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Trigger.h"

enum HallsOfStoneIDs
{
    // Krystallus
    SPELL_GROUND_SLAM               = 50827,
    DEBUFF_GROUND_SLAM              = 50833,

    // Sjonnir The Ironshaper
    SPELL_LIGHTNING_RING_N          = 50840,
    SPELL_LIGHTNING_RING_H          = 59848,

    // Tribunal of Ages
    NPC_SEARING_GAZE_TRIGGER        = 28265,
};

#define SPELL_LIGHTNING_RING        DUNGEON_MODE(bot, SPELL_LIGHTNING_RING_N, SPELL_LIGHTNING_RING_H)

Unit* FindTribunalLosReacquireTarget(PlayerbotAI* botAI);
Unit* FindTribunalRangedLosRegainTarget(PlayerbotAI* botAI);

class KrystallusGroundSlamTrigger : public Trigger
{
public:
    KrystallusGroundSlamTrigger(PlayerbotAI* ai) : Trigger(ai, "krystallus ground slam") {}
    bool IsActive() override;
};

class SjonnirLightningRingTrigger : public Trigger
{
public:
    SjonnirLightningRingTrigger(PlayerbotAI* ai) : Trigger(ai, "sjonnir lightning ring") {}
    bool IsActive() override;
};

class TribunalLosReacquireTrigger : public Trigger
{
public:
    TribunalLosReacquireTrigger(PlayerbotAI* ai) : Trigger(ai, "tribunal los reacquire") {}
    bool IsActive() override;
};

// Telemetry only: never active. Once per second, logs a ranged DPS bot's target and
// LOS state so idle stretches can be split into "no target" vs "target not castable".
class TribunalRangedIdleProbeTrigger : public Trigger
{
public:
    TribunalRangedIdleProbeTrigger(PlayerbotAI* ai) : Trigger(ai, "tribunal ranged idle probe") {}
    bool IsActive() override;

private:
    uint32 lastLogMs = 0;
};

// Ranged DPS with an empty (LOS-filtered) attacker list while a Tribunal add hits the group
// out of its sight: move back toward the main tank to regain LOS.
class TribunalRangedLosRegainTrigger : public Trigger
{
public:
    TribunalRangedLosRegainTrigger(PlayerbotAI* ai) : Trigger(ai, "tribunal ranged los regain") {}
    bool IsActive() override;
};

class TribunalSearingGazeTrigger : public Trigger
{
public:
    TribunalSearingGazeTrigger(PlayerbotAI* ai) : Trigger(ai, "tribunal searing gaze") {}
    bool IsActive() override;
};

#endif
