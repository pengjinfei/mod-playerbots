/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AKActions.h"
#include "Playerbots.h"

bool AttackNadoxGuardianAction::Execute(Event /*event*/)
{
    Unit* target = AI_VALUE2(Unit*, "find target", "ahn'kahar guardian");
    if (!target || AI_VALUE(Unit*, "current target") == target)
    {
        return false;
    }
    return Attack(target);
}

bool AttackJedogaVolunteerAction::Execute(Event /*event*/)
{
    Unit* target = nullptr;
    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit) // Null check for safety
        {
            continue; // Skip null or invalid units
        }
        if (unit && unit->GetEntry() == NPC_TWILIGHT_VOLUNTEER)
        {
            target = unit;
            break;
        }
    }

    if (!target || AI_VALUE(Unit*, "current target") == target)
    {
        return false;
    }
    return Attack(target);
}

// The casters first, as players kill them: Jedoga herself hits far less than ten of them casting Fireball.
bool AttackJedogaWorshipperAction::Execute(Event /*event*/)
{
    Unit* worshipper = NearestJedogaWorshipper(bot);
    if (!worshipper || AI_VALUE(Unit*, "current target") == worshipper)
        return false;
    return Attack(worshipper);
}

// Out of the flame spheres' way, as players do: to the side none of them walks to, until they are gone.
bool AvoidFlameSphereAction::Execute(Event /*event*/)
{
    float x, y, z;
    if (!FlameSphereSafePoint(bot, x, y, z))
        return false;
    // A straight step on Taldaram's platform: it is a game object the navmesh does not have, and every path search
    // to the safe side failed there (run 1977).
    Creature* taldaram = bot->FindNearestCreature(NPC_TALDARAM_OK, TALDARAM_SPHERE_SIGHT);
    Unit* victim = taldaram ? taldaram->GetVictim() : nullptr;
    LOG_DEBUG("playerbots", "flame-sphere bot={} from=({:.1f},{:.1f}) to=({:.1f},{:.1f}) victim={} los={}",
              bot->GetName(), bot->GetPositionX(), bot->GetPositionY(), x, y, victim ? victim->GetName() : "-",
              bot->IsWithinLOS(x, y, z + 2.0f));
    bot->GetMotionMaster()->MovePoint(0, x, y, z, FORCED_MOVEMENT_NONE, 0.0f, 0.0f, false, true);
    return true;
}

bool AvoidShadowCrashAction::Execute(Event /*event*/)
{
    // Could check all enemy units in range as it's possible to pull multiple of these mobs.
    // They should really be killed 1 by 1, multipulls are messy so we just handle singles for now
    Unit* unit = AI_VALUE2(Unit*, "find target", "forgotten one");
    if (!unit) { return false; }

    Unit* victim = nullptr;
    float radius = 10.0f;
    float targetDist = radius + 2.0f;

    // Actively move if targeted by a shadow crash.
    // Spell check not needed, they don't have any other non-instant casts
    if (unit->HasUnitState(UNIT_STATE_CASTING)) // && unit->FindCurrentSpellBySpellId(SPELL_SHADOW_CRASH))
    {
        // This doesn't seem to avoid casts very well, perhaps because this isn't checked while allies are casting.
        // TODO: Revisit if this is an issue in heroics, otherwise ignore shadow crashes for the most part.
        victim = botAI->GetUnit(unit->GetTarget());
        float distance = bot->GetExactDist2d(victim->GetPosition());

        if (victim && distance < radius)
        {
            return MoveAway(victim, targetDist - distance);
        }
    }

    // Otherwise ranged members passively spread, to avoid AoE overlap
    if (botAI->IsMelee(bot)) { return false; }

    GuidVector members = AI_VALUE(GuidVector, "group members");
    for (auto& member : members)
    {
        Unit* unit = botAI->GetUnit(member);
        if (!unit || bot->GetGUID() == member)
        {
            continue;
        }
        float currentDist = bot->GetExactDist2d(botAI->GetUnit(member));
        if (currentDist < radius)
        {
            return MoveAway(botAI->GetUnit(member), targetDist - currentDist);
        }
    }
    return false;
}
