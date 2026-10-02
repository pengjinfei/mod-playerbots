/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AKTriggers.h"
#include "AiObjectContext.h"
#include "Playerbots.h"
#include "MoveSpline.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

Unit* NearestFlameSphere(Player* bot, float range)
{
    Unit* nearest = nullptr;
    for (uint32 entry : {NPC_FLAME_SPHERE_1, NPC_FLAME_SPHERE_2, NPC_FLAME_SPHERE_3})
        if (Creature* sphere = bot->FindNearestCreature(entry, range))
            if (!nearest || bot->GetDistance(sphere) < bot->GetDistance(nearest))
                nearest = sphere;
    return nearest;
}

// Where the spheres start from and the way the first walks, once they are summoned.
static bool FlameSphereLayout(Player* bot, float& startX, float& startY, float& way)
{
    // Moving: the first one's way straight from its spawn point; the others' turned back by 90 degrees.
    static constexpr std::array<std::pair<uint32, float>, 3> spheres = {{
        {NPC_FLAME_SPHERE_1, 0.0f}, {NPC_FLAME_SPHERE_2, -float(M_PI) / 2.0f}, {NPC_FLAME_SPHERE_3, float(M_PI) / 2.0f}}};
    for (auto const& [entry, turn] : spheres)
    {
        Creature* sphere = bot->FindNearestCreature(entry, TALDARAM_SPHERE_SIGHT);
        if (!sphere || sphere->movespline->Finalized())
            continue;
        // Straight walks of 25 yd (boss_prince_taldaram DATA_SPHERE_DISTANCE): the spawn point is that far back.
        G3D::Vector3 const destination = sphere->movespline->FinalDestination();
        float const heading = std::atan2(destination.y - sphere->GetPositionY(), destination.x - sphere->GetPositionX());
        startX = destination.x - TALDARAM_SPHERE_WALK * std::cos(heading);
        startY = destination.y - TALDARAM_SPHERE_WALK * std::sin(heading);
        way = heading + turn;
        return true;
    }
    // Not moving yet: they sit where they spawned for 3 s and burn everyone around once they set off - the group by
    // Taldaram took 25k each in 2 s (run 1979). The first one will walk towards where his victim, the tank, stood when
    // he cast them (boss_prince_taldaram SetVictimPos), so the way is known before they move.
    Creature* first = bot->FindNearestCreature(NPC_FLAME_SPHERE_1, TALDARAM_SPHERE_SIGHT);
    Creature* taldaram = bot->FindNearestCreature(NPC_TALDARAM_OK, TALDARAM_SPHERE_SIGHT);
    Unit* victim = taldaram ? taldaram->GetVictim() : nullptr;
    if (!first || !victim)
        return false;
    startX = first->GetPositionX();
    startY = first->GetPositionY();
    way = first->GetAngle(victim);
    return true;
}

bool FlameSphereSafePoint(Player* bot, float& x, float& y, float& z)
{
    float startX, startY, way;
    if (!FlameSphereLayout(bot, startX, startY, way))
        return false;
    // Clear of all three ways: 20 yd straight behind the first one's way, or 30 yd out between it and a side one's
    // (21 yd off both; they walk only 25). The nearest of these whose straight way there does not pass their spawn
    // point: the tank, standing where the first one walks, crossed it to reach the back and was burnt (run 1987).
    struct Spot
    {
        float angle;
        float distance;
    };
    static constexpr std::array<Spot, 3> spots = {{{float(M_PI), TALDARAM_SPHERE_SAFE_DISTANCE},
                                                   {float(M_PI) / 4.0f, TALDARAM_SPHERE_DIAGONAL_DISTANCE},
                                                   {-float(M_PI) / 4.0f, TALDARAM_SPHERE_DIAGONAL_DISTANCE}}};
    float best = std::numeric_limits<float>::max();
    for (Spot const& spot : spots)
    {
        float const sx = startX + spot.distance * std::cos(way + spot.angle);
        float const sy = startY + spot.distance * std::sin(way + spot.angle);
        // Closest approach of the straight way there to the spawn point.
        float const dx = sx - bot->GetPositionX(), dy = sy - bot->GetPositionY();
        float const length2 = dx * dx + dy * dy;
        float t = length2 > 0.0f ? ((startX - bot->GetPositionX()) * dx + (startY - bot->GetPositionY()) * dy) / length2
                                 : 0.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        float const pass = std::hypot(bot->GetPositionX() + t * dx - startX, bot->GetPositionY() + t * dy - startY);
        float const cost = std::sqrt(length2) + (pass < TALDARAM_SPHERE_SPAWN_CLEARANCE ? 1000.0f : 0.0f);
        if (cost < best)
        {
            best = cost;
            x = sx;
            y = sy;
        }
    }
    z = bot->GetPositionZ();
    return true;
}

// On a moving sphere's way (from where it is to where it walks), or within reach of it.
static bool OnFlameSpherePath(Player* bot)
{
    for (uint32 entry : {NPC_FLAME_SPHERE_1, NPC_FLAME_SPHERE_2, NPC_FLAME_SPHERE_3})
    {
        Creature* sphere = bot->FindNearestCreature(entry, TALDARAM_SPHERE_SIGHT);
        if (!sphere || sphere->movespline->Finalized())
            continue;
        G3D::Vector3 const to = sphere->movespline->FinalDestination();
        float const ax = sphere->GetPositionX(), ay = sphere->GetPositionY();
        float const dx = to.x - ax, dy = to.y - ay;
        float const length2 = dx * dx + dy * dy;
        float t = length2 > 0.0f ? ((bot->GetPositionX() - ax) * dx + (bot->GetPositionY() - ay) * dy) / length2 : 0.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        if (bot->GetExactDist2d(ax + t * dx, ay + t * dy) < TALDARAM_SPHERE_PATH_WIDTH)
            return true;
    }
    return false;
}

bool TaldaramFlameSphereTrigger::IsActive()
{
    float x, y, z;
    if (!FlameSphereSafePoint(bot, x, y, z) || bot->GetExactDist2d(x, y) <= 3.0f)
        return false;
    // Spawned and still: everyone near them gets away.
    bool moving = false;
    for (uint32 entry : {NPC_FLAME_SPHERE_1, NPC_FLAME_SPHERE_2, NPC_FLAME_SPHERE_3})
        if (Creature* sphere = bot->FindNearestCreature(entry, TALDARAM_SPHERE_SIGHT))
            moving = moving || !sphere->movespline->Finalized();
    if (!moving)
        return true;
    // The tank takes Taldaram there and melee go with him; casters only step out of a sphere's way and go on.
    return botAI->IsTank(bot) || botAI->IsMelee(bot) || OnFlameSpherePath(bot);
}

Unit* NearestJedogaWorshipper(Player* bot)
{
    std::list<Creature*> worshippers;
    bot->GetCreatureListWithEntryInGrid(worshippers, NPC_TWILIGHT_WORSHIPPER_OK, 40.0f);
    Unit* nearest = nullptr;
    for (Creature* worshipper : worshippers)
    {
        if (!worshipper->IsAlive() || !worshipper->IsInCombat() || !bot->IsValidAttackTarget(worshipper))
            continue;
        if (!nearest || bot->GetDistance(worshipper) < bot->GetDistance(nearest))
            nearest = worshipper;
    }
    return nearest;
}

bool JedogaWorshipperTrigger::IsActive()
{
    if (botAI->IsTank(bot) || botAI->IsHeal(bot))
        return false;
    if (!AI_VALUE2(Unit*, "find target", "jedoga shadowseeker"))
        return false;
    Unit* worshipper = NearestJedogaWorshipper(bot);
    return worshipper && AI_VALUE(Unit*, "current target") != worshipper;
}

bool NadoxGuardianTrigger::IsActive()
{
    if (botAI->IsHeal(bot)) { return false; }

    Unit* boss = AI_VALUE2(Unit*, "find target", "elder nadox");
    Unit* guardian = AI_VALUE2(Unit*, "find target", "ahn'kahar guardian");

    return boss && guardian;
}

bool JedogaVolunteerTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "jedoga shadowseeker");
    if (!boss) { return false; }

    // Volunteer is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_TWILIGHT_VOLUNTEER)
        {
            return true;
        }
    }
    return false;
}

bool ShadowCrashTrigger::IsActive()
{
    Unit* unit = AI_VALUE2(Unit*, "find target", "forgotten one");
    if (!unit) { return false; }

    return !botAI->IsMelee(bot);
}
