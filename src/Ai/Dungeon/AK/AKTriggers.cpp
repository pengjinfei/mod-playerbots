/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AKTriggers.h"
#include "AiObjectContext.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "MoveSpline.h"
#include "Spell.h"
#include "Timer.h"

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
    return false;
}

// Where his victim stood as he began Conjure Flame Sphere: the first sphere walks straight there (the script keeps that
// position, SetVictimPos), so the way is known before they move. One map's bots share a thread.
struct SphereCast
{
    uint32 instanceId = 0;
    ObjectGuid taldaram;
    float x = 0.0f, y = 0.0f;
    uint32 atMs = 0;
};
static thread_local SphereCast lastSphereCast;

static void ObserveSphereCast(Player* bot)
{
    Creature* taldaram = bot->FindNearestCreature(NPC_TALDARAM_OK, TALDARAM_SPHERE_SIGHT);
    if (!taldaram)
        return;
    Spell* const spell = taldaram->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    Unit* const victim = taldaram->GetVictim();
    if (!spell || spell->m_spellInfo->Id != SPELL_CONJURE_FLAME_SPHERE_OK || !victim)
        return;
    uint32 const now = getMSTime();
    // The first look at this cast: the victim may step away before it ends.
    if (lastSphereCast.instanceId == taldaram->GetInstanceId() && lastSphereCast.taldaram == taldaram->GetGUID() &&
        getMSTimeDiff(lastSphereCast.atMs, now) < TALDARAM_SPHERE_CAST_SAME_MS)
        return;
    lastSphereCast = {taldaram->GetInstanceId(), taldaram->GetGUID(), victim->GetPositionX(), victim->GetPositionY(), now};
}

// The way of still spheres from the cast seen: a way guessed from where the tank stands once they are out sent the
// group to one side, then back through the burn when they set off the other way (runs 2023, 2061).
static bool FlameSphereStillLayout(Player* bot, float& startX, float& startY, float& way)
{
    Creature* first = bot->FindNearestCreature(NPC_FLAME_SPHERE_1, TALDARAM_SPHERE_SIGHT);
    if (!first || !first->movespline->Finalized() || lastSphereCast.instanceId != first->GetInstanceId() ||
        getMSTimeDiff(lastSphereCast.atMs, getMSTime()) > TALDARAM_SPHERE_CAST_VALID_MS)
        return false;
    startX = first->GetPositionX();
    startY = first->GetPositionY();
    way = first->GetAngle(lastSphereCast.x, lastSphereCast.y);
    return true;
}

// Still at their spawn: they sit there 3 s and burn everyone round them once they set off - the group by Taldaram took
// 25k each in 2 s (run 1979). Which way the first one walks is not known yet: it heads for where his victim stood
// when he cast them, and a way guessed from where the tank stands now sent the group to one side, then back through
// the burn when the spheres set off the other way (runs 2023, 2061). Straight out of their reach instead; the way is
// read off the moving spheres after that.
static bool FlameSphereStillAway(Player* bot, float& x, float& y)
{
    Creature* first = bot->FindNearestCreature(NPC_FLAME_SPHERE_1, TALDARAM_SPHERE_SIGHT);
    if (!first || !first->movespline->Finalized())
        return false;
    float const distance = bot->GetExactDist2d(first);
    if (distance >= TALDARAM_SPHERE_STILL_CLEARANCE)
        return false;
    float const angle = distance > 0.5f ? first->GetAngle(bot) : bot->GetOrientation() + float(M_PI);
    // Not off the platform: straight out 30 yd took the mage over its edge to the floor below (run 2068).
    for (float const reach : {TALDARAM_SPHERE_STILL_CLEARANCE, TALDARAM_SPHERE_SAFE_DISTANCE})
    {
        x = first->GetPositionX() + reach * std::cos(angle);
        y = first->GetPositionY() + reach * std::sin(angle);
        float const floor =
            bot->GetMap()->GetHeight(bot->GetPhaseMask(), x, y, bot->GetPositionZ() + 2.0f, true, 6.0f);
        if (floor > INVALID_HEIGHT && std::fabs(floor - bot->GetPositionZ()) < 3.0f)
            return true;
    }
    return true;
}

bool FlameSphereSafePoint(Player* bot, float& x, float& y, float& z)
{
    float startX, startY, way;
    if (!FlameSphereLayout(bot, startX, startY, way) && !FlameSphereStillLayout(bot, startX, startY, way))
    {
        if (!FlameSphereStillAway(bot, x, y))
            return false;
        float const floor = bot->GetMap()->GetHeight(bot->GetPhaseMask(), x, y, bot->GetPositionZ() + 2.0f, true, 6.0f);
        z = floor > INVALID_HEIGHT && std::fabs(floor - bot->GetPositionZ()) < 3.0f ? floor : bot->GetPositionZ();
        return true;
    }
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
    float bestZ = bot->GetPositionZ();
    for (Spot const& spot : spots)
    {
        float const sx = startX + spot.distance * std::cos(way + spot.angle);
        float const sy = startY + spot.distance * std::sin(way + spot.angle);
        // Only onto floor at the bot's height (the platform is a game object, which the height search sees): a spot
        // past its edge took all five down to z 1 off the navmesh, and after Taldaram nobody could walk out of there
        // (run 2020, stalled). A spot over no floor costs more than any other, so one is still picked.
        float const floor =
            bot->GetMap()->GetHeight(bot->GetPhaseMask(), sx, sy, bot->GetPositionZ() + 2.0f, true, 6.0f);
        bool const onFloor = floor > INVALID_HEIGHT && std::fabs(floor - bot->GetPositionZ()) < 3.0f;
        // Closest approach of the straight way there to the spawn point.
        float const dx = sx - bot->GetPositionX(), dy = sy - bot->GetPositionY();
        float const length2 = dx * dx + dy * dy;
        float t = length2 > 0.0f ? ((startX - bot->GetPositionX()) * dx + (startY - bot->GetPositionY()) * dy) / length2
                                 : 0.0f;
        t = std::clamp(t, 0.0f, 1.0f);
        float const pass = std::hypot(bot->GetPositionX() + t * dx - startX, bot->GetPositionY() + t * dy - startY);
        float const cost = std::sqrt(length2) + (pass < TALDARAM_SPHERE_SPAWN_CLEARANCE ? 1000.0f : 0.0f) +
                           (onFloor ? 0.0f : 2000.0f);
        if (cost < best)
        {
            best = cost;
            x = sx;
            y = sy;
            bestZ = onFloor ? floor : bot->GetPositionZ();
        }
    }
    z = bestZ;
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
    ObserveSphereCast(bot);
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

// Taldaram while he drains someone with the Embrace of the Vampyr (55959, heroic 59513): it breaks after 40k damage on
// heroic. He vanishes into it, the bots lost him as a target and hit him for 3k in 5 s instead of 20-50k, and two of
// the group were drained to death (run 2066). As players do: everyone on him at once.
Unit* TaldaramEmbracing(Player* bot)
{
    Creature* taldaram = bot->FindNearestCreature(NPC_TALDARAM_OK, TALDARAM_SPHERE_SIGHT);
    if (!taldaram || !taldaram->IsAlive())
        return nullptr;
    Spell* const channel = taldaram->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    Spell* const cast = taldaram->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    for (Spell* spell : {channel, cast})
        if (spell && (spell->m_spellInfo->Id == SPELL_EMBRACE_OF_THE_VAMPYR_OK ||
                      spell->m_spellInfo->Id == SPELL_EMBRACE_OF_THE_VAMPYR_H_OK))
            return bot->IsValidAttackTarget(taldaram) ? taldaram : nullptr;
    return nullptr;
}

bool TaldaramEmbraceTrigger::IsActive()
{
    if (botAI->IsHeal(bot))
        return false;
    Unit* taldaram = TaldaramEmbracing(bot);
    return taldaram && AI_VALUE(Unit*, "current target") != taldaram;
}

Unit* NearestJedogaWorshipper(Player* bot)
{
    std::list<Creature*> worshippers;
    bot->GetCreatureListWithEntryInGrid(worshippers, NPC_TWILIGHT_WORSHIPPER_OK, 40.0f);
    Unit* nearest = nullptr;
    for (Creature* worshipper : worshippers)
    {
        // In sight only: the attack refuses one that is not, and the mage, given one behind the altar, failed nine
        // times out of eleven and stayed on Jedoga (run 2042).
        if (!worshipper->IsAlive() || !worshipper->IsInCombat() || !bot->IsValidAttackTarget(worshipper) ||
            !bot->IsWithinLOSInMap(worshipper))
            continue;
        // The most hurt one, so everyone ends on the same: each on its nearest, the DPS spread over nine worshippers,
        // switched as the nearest changed and killed none before the group fell (run 2036).
        if (!nearest || worshipper->GetHealthPct() < nearest->GetHealthPct() ||
            (worshipper->GetHealthPct() == nearest->GetHealthPct() &&
             bot->GetDistance(worshipper) < bot->GetDistance(nearest)))
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
    // On one already: stay on it until it dies.
    Unit* current = AI_VALUE(Unit*, "current target");
    if (current && current->IsAlive() && current->GetEntry() == NPC_TWILIGHT_WORSHIPPER_OK)
        return false;
    return NearestJedogaWorshipper(bot) != nullptr;
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
