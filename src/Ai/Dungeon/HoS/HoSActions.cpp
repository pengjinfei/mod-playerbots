/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoSActions.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include "Group.h"
#include "Log.h"
#include "Playerbots.h"
#include "Timer.h"

bool ShatterSpreadAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "krystallus");
    if (!boss) { return false; }

    float radius = 40.0f;
    Unit* closestMember = nullptr;

    GuidVector members = AI_VALUE(GuidVector, "group members");
    for (auto& member : members)
    {
        Unit* unit = botAI->GetUnit(member);
        if (!unit || bot->GetGUID() == member)
        {
            continue;
        }
        if (!closestMember || bot->GetExactDist2d(unit) < bot->GetExactDist2d(closestMember))
        {
            closestMember = unit;
        }
    }

    if (!closestMember || bot->GetExactDist2d(closestMember) >= radius)
        return false;

    // Spread on Krystallus' platform only: stepping away from the nearest member without bounds took bots over its
    // edge, they fell among the Crystalline Shardlings below and died where the group could not reach them (full
    // runs 2333, 2334). Of the points a step away, the one farthest from every member, within reach of the boss and
    // on the bot's own level (no drop). Bound to his level instead, bots standing 30 yd down the slope had no point to
    // go to, stood together and four died to one Shatter (run 2341).
    constexpr float SPREAD_STEP = 5.0f;
    constexpr float SPREAD_BOSS_RANGE = 35.0f;
    constexpr float SPREAD_MAX_DROP = 1.0f;
    float bestScore = -1.0f;
    float bestX = 0.0f;
    float bestY = 0.0f;
    for (int i = 0; i < 12; ++i)
    {
        float const angle = i * float(M_PI) / 6.0f;
        float const x = bot->GetPositionX() + SPREAD_STEP * std::cos(angle);
        float const y = bot->GetPositionY() + SPREAD_STEP * std::sin(angle);
        if (boss->GetExactDist2d(x, y) > SPREAD_BOSS_RANGE)
            continue;
        float const ground = bot->GetMap()->GetHeight(bot->GetPhaseMask(), x, y, bot->GetPositionZ() + 2.0f, true, 6.0f);
        // Never lower than where the bot stands: step by step down the slope leads to the Shardlings below.
        if (ground <= INVALID_HEIGHT || ground < std::min(bot->GetPositionZ(), boss->GetPositionZ()) - SPREAD_MAX_DROP)
            continue;
        float nearest = std::numeric_limits<float>::max();
        for (auto& member : members)
            if (Unit* unit = botAI->GetUnit(member))
                if (unit != bot)
                    nearest = std::min(nearest, unit->GetExactDist2d(x, y));
        if (nearest > bestScore)
        {
            bestScore = nearest;
            bestX = x;
            bestY = y;
        }
    }
    if (bestScore <= bot->GetExactDist2d(closestMember))
        return false;
    return MoveTo(bot->GetMapId(), bestX, bestY, bot->GetPositionZ(), false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT);
}

bool AvoidLightningRingAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "sjonnir the ironshaper");
    if (!boss) { return false; }

    float distance = bot->GetExactDist2d(boss->GetPosition());
    float radius = 10.0f;
    float distanceExtra = 2.0f;

    if (distance < radius + distanceExtra)
    {
        return MoveAway(boss, radius + distanceExtra - distance);
    }

    return false;
}

bool TribunalLosReacquireAction::Execute(Event /*event*/)
{
    Unit* target = FindTribunalLosReacquireTarget(botAI);
    if (!target)
        return false;

    Unit* victim = target->GetVictim();
    float healerDistance = -1.0f;
    if (Group* group = bot->GetGroup())
    {
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || !member->IsAlive() || member->GetMapId() != bot->GetMapId() || !PlayerbotAI::IsHeal(member))
                continue;

            float const distance = target->GetExactDist(member);
            if (healerDistance < 0.0f || distance < healerDistance)
                healerDistance = distance;
        }
    }

    float const botX = bot->GetPositionX();
    float const botY = bot->GetPositionY();
    float const botZ = bot->GetPositionZ();
    float const targetX = target->GetPositionX();
    float const targetY = target->GetPositionY();
    float const targetZ = target->GetPositionZ();
    bool const moved = ReachCombatTo(target);
    LOG_INFO("playerbots", "tribunal-los-reacquire app_ms={} bot={} target={} victim_low={} healer_dist={:.2f} "
                           "bot_pos={:.2f},{:.2f},{:.2f} target_pos={:.2f},{:.2f},{:.2f} moved={}",
             getMSTime(), bot->GetName(), target->GetEntry(), victim ? victim->GetGUID().GetCounter() : 0,
             healerDistance, botX, botY, botZ, targetX, targetY, targetZ, moved);
    return moved;
}

bool TribunalRangedLosRegainAction::Execute(Event /*event*/)
{
    Unit* target = FindTribunalRangedLosRegainTarget(botAI);
    if (!target)
        return false;

    // The main tank is in melee with the fight, so standing near it restores LOS
    // without choosing a target or threat for the bot.
    Unit* tank = AI_VALUE(Unit*, "main tank");
    if (!tank || !tank->IsAlive() || tank->GetMapId() != bot->GetMapId() || bot->GetExactDist(tank) > 60.0f)
    {
        LOG_INFO("playerbots", "tribunal-ranged-los-regain app_ms={} bot={} target={} target_dist={:.1f} "
                               "tank=none moved=false",
                 getMSTime(), bot->GetName(), target->GetEntry(), bot->GetExactDist(target));
        return false;
    }

    float const tankDistance = bot->GetExactDist(tank);
    bool const moved = MoveNear(tank, 8.0f);
    LOG_INFO("playerbots", "tribunal-ranged-los-regain app_ms={} bot={} target={} target_dist={:.1f} "
                           "tank_dist={:.1f} bot_pos={:.1f},{:.1f},{:.1f} moved={}",
             getMSTime(), bot->GetName(), target->GetEntry(), bot->GetExactDist(target), tankDistance,
             bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), moved);
    return moved;
}

bool TribunalFleeSearingGazeAction::Execute(Event /*event*/)
{
    Creature* gaze = bot->FindNearestCreature(NPC_SEARING_GAZE_TRIGGER, 5.0f);
    if (!gaze)
        return false;

    constexpr float safeDistance = 12.0f;
    return MoveAway(gaze, safeDistance - bot->GetExactDist2d(gaze));
}
