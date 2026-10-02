/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AKTRIGGERS_H
#define PLAYERBOTS_AKTRIGGERS_H

#include "DungeonStrategyUtils.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Trigger.h"

enum OldKingdomIDs
{
    // Elder Nadox
    BUFF_GUARDIAN_AURA                 = 56153,

    // Prince Taldaram: the three flame spheres (one per entry), moving out 25 yd from him
    NPC_TALDARAM_OK                    = 29308,
    NPC_FLAME_SPHERE_1                 = 30106,
    NPC_FLAME_SPHERE_2                 = 31686,
    NPC_FLAME_SPHERE_3                 = 31687,

    // Jedoga Shadowseeker
    NPC_TWILIGHT_VOLUNTEER             = 30385,

    // Forgotten One(s)
    SPELL_SHADOW_CRASH_N               = 60833,
    SPELL_SHADOW_CRASH_H               = 60848,
};

#define SPELL_SHADOW_CRASH             DUNGEON_MODE(bot, SPELL_SHADOW_CRASH_N, SPELL_SHADOW_CRASH_H)

class NadoxGuardianTrigger : public Trigger
{
public:
    NadoxGuardianTrigger(PlayerbotAI* ai) : Trigger(ai, "elder nadox guardian") {}
    bool IsActive() override;
};

class JedogaVolunteerTrigger : public Trigger
{
public:
    JedogaVolunteerTrigger(PlayerbotAI* ai) : Trigger(ai, "jedoga volunteer") {}
    bool IsActive() override;
};

// Taldaram's flame spheres spawn on him; one then walks 25 yd towards a target, the other two at +-90 degrees to
// it. Their burn (59509 on heroic) killed four of five standing together in them (Ahn'kahet, runs 1970/1971/1976);
// stepping away from the nearest one did not hold, melee walked straight back to the boss. The side opposite the
// first sphere's way is clear: everyone waits there until they are gone.
constexpr float TALDARAM_SPHERE_SIGHT = 40.0f;
constexpr float TALDARAM_SPHERE_SAFE_DISTANCE = 20.0f;  // from where they spawned: their burn reached 16 yd (run 1983)
constexpr float TALDARAM_SPHERE_WALK = 25.0f;           // how far each walks
constexpr float TALDARAM_SPHERE_PATH_WIDTH = 18.0f;     // this close to a sphere's way is in it
Unit* NearestFlameSphere(Player* bot, float range);
// The point to wait at while the spheres walk, or false while their way is not known yet (not moving).
bool FlameSphereSafePoint(Player* bot, float& x, float& y, float& z);

class TaldaramFlameSphereTrigger : public Trigger
{
public:
    TaldaramFlameSphereTrigger(PlayerbotAI* ai) : Trigger(ai, "taldaram flame sphere") {}
    bool IsActive() override;
};

class ShadowCrashTrigger : public Trigger
{
public:
    ShadowCrashTrigger(PlayerbotAI* ai) : Trigger(ai, "shadow crash") {}
    bool IsActive() override;
};

#endif
