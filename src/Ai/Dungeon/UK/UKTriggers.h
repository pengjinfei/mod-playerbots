/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_UKTRIGGERS_H
#define PLAYERBOTS_UKTRIGGERS_H

#include "DungeonStrategyUtils.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Trigger.h"

enum UtgardeKeepIDs
{
    // Prince Keleseth
    SPELL_FROST_TOMB               = 48400,
    NPC_FROST_TOMB                 = 23965,

    // Ingvar the Plunderer
    SPELL_STAGGERING_ROAR_N         = 42708,
    SPELL_STAGGERING_ROAR_H         = 59708,
    SPELL_SMASH_N                   = 42669,
    SPELL_SMASH_H                   = 59706,
    SPELL_DREADFUL_ROAR_N           = 42729,
    SPELL_DREADFUL_ROAR_H           = 59734,
    SPELL_WOE_STRIKE_N              = 42730,
    SPELL_WOE_STRIKE_H              = 59735,
    SPELL_DARK_SMASH_N              = 42723,
    SPELL_DARK_SMASH_H              = 59709,
    INGVAR_UNDEAD_DISPLAY_ID         = 26351,
    NPC_THROW                       = 23997,
};

// Measured from Spell.dbc / SpellRadius.dbc, not tuned by hand:
//  - 42669/59706 and 42723/59709 effect 0 use radius index 13 = 10 yd. That is the
//    front-cone one-shot (37,000-43,000 in P1, 26,250-33,750 in heroic P2), so any
//    member farther than 10 yd from Ingvar cannot be selected by it at all.
//  - the thrown axe carries aura 42750, which ticks 42751 once per second with
//    radius index 8 = 5 yd around the axe. Keeping members more than 5 yd apart
//    means one axe can only ever reach the member it landed on.
// 59709 effect 1 (stun, radius 200 yd) and effect 2 (damage, radius 200 yd), and
// Dreadful Roar (radius 60 yd), have no positional answer and are left alone.
constexpr float kIngvarSmashConeRadius = 10.0f;
constexpr float kIngvarShadowAxeRadius = 5.0f;
constexpr float kIngvarSpreadRadius = kIngvarShadowAxeRadius + 3.0f;
constexpr float kIngvarRangedClearance = kIngvarSmashConeRadius + 3.0f;

class Player;
class PlayerbotAI;
class Unit;

// Nearest living group member crowding this bot inside the axe spread radius, or
// nullptr. Shared so the trigger and its action cannot drift apart.
Unit* FindIngvarCrowdingMember(PlayerbotAI* botAI, Player* bot);

#define SPELL_STAGGERING_ROAR       DUNGEON_MODE(bot, SPELL_STAGGERING_ROAR_N, SPELL_STAGGERING_ROAR_H)
#define SPELL_DREADFUL_ROAR         DUNGEON_MODE(bot, SPELL_DREADFUL_ROAR_N, SPELL_DREADFUL_ROAR_H)
#define SPELL_SMASH                 DUNGEON_MODE(bot, SPELL_SMASH_N, SPELL_SMASH_H)
#define SPELL_DARK_SMASH            DUNGEON_MODE(bot, SPELL_DARK_SMASH_N, SPELL_DARK_SMASH_H)

class KelesethFrostTombTrigger : public Trigger
{
public:
    KelesethFrostTombTrigger(PlayerbotAI* ai) : Trigger(ai, "keleseth frost tomb") {}
    bool IsActive() override;
};

class DalronnDpsTrigger : public Trigger
{
public:
    DalronnDpsTrigger(PlayerbotAI* ai) : Trigger(ai, "dalronn dps") {}
    bool IsActive() override;
};

class IngvarDreadfulRoarTrigger : public Trigger
{
public:
    IngvarDreadfulRoarTrigger(PlayerbotAI* ai) : Trigger(ai, "ingvar dreadful roar") {}
    bool IsActive() override;
};

class IngvarSmashTankTrigger : public Trigger
{
public:
    IngvarSmashTankTrigger(PlayerbotAI* ai) : Trigger(ai, "ingvar smash tank") {}
    bool IsActive() override;
};

class IngvarDarkSmashNonTankTrigger : public Trigger
{
public:
    IngvarDarkSmashNonTankTrigger(PlayerbotAI* ai) : Trigger(ai, "ingvar dark smash non tank") {}
    bool IsActive() override;
};

class IngvarContactClearanceTrigger : public Trigger
{
public:
    IngvarContactClearanceTrigger(PlayerbotAI* ai) : Trigger(ai, "ingvar contact clearance") {}
    bool IsActive() override;
};

class NotBehindIngvarTrigger : public Trigger
{
public:
    NotBehindIngvarTrigger(PlayerbotAI* ai) : Trigger(ai, "not behind ingvar") {}
    bool IsActive() override;
};

class IngvarShadowAxeTrigger : public Trigger
{
public:
    IngvarShadowAxeTrigger(PlayerbotAI* ai) : Trigger(ai, "ingvar shadow axe") {}
    bool IsActive() override;
};

class IngvarRangedClearanceTrigger : public Trigger
{
public:
    IngvarRangedClearanceTrigger(PlayerbotAI* ai) : Trigger(ai, "ingvar ranged clearance") {}
    bool IsActive() override;
};

class IngvarSpreadTrigger : public Trigger
{
public:
    IngvarSpreadTrigger(PlayerbotAI* ai) : Trigger(ai, "ingvar spread") {}
    bool IsActive() override;
};

#endif
