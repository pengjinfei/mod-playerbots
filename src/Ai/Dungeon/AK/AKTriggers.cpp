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

Unit* NearestFlameSphere(Player* bot, float range)
{
    Unit* nearest = nullptr;
    for (uint32 entry : {NPC_FLAME_SPHERE_1, NPC_FLAME_SPHERE_2, NPC_FLAME_SPHERE_3})
        if (Creature* sphere = bot->FindNearestCreature(entry, range))
            if (!nearest || bot->GetDistance(sphere) < bot->GetDistance(nearest))
                nearest = sphere;
    return nearest;
}

bool FlameSphereSafePoint(Player* bot, float& x, float& y, float& z)
{
    // The way the spheres walk: the first one's straight from its spawn point; the others' turned back by 90 degrees.
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
        float const startX = destination.x - TALDARAM_SPHERE_WALK * std::cos(heading);
        float const startY = destination.y - TALDARAM_SPHERE_WALK * std::sin(heading);
        float const way = heading + turn;
        x = startX + TALDARAM_SPHERE_SAFE_DISTANCE * std::cos(way + float(M_PI));
        y = startY + TALDARAM_SPHERE_SAFE_DISTANCE * std::sin(way + float(M_PI));
        z = sphere->GetPositionZ();
        return true;
    }
    return false;
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
    // The tank takes Taldaram there and melee go with him; casters only step out of a sphere's way and go on.
    return botAI->IsTank(bot) || botAI->IsMelee(bot) || OnFlameSpherePath(bot);
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
