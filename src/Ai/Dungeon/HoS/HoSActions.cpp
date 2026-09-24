/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoSActions.h"
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

    if (closestMember && bot->GetExactDist2d(closestMember) < radius)
    {
        // Move in small increments so course can be corrected, otherwise two bots may
        // run in the same direction and not adjust until they reach the destination
        // return MoveAway(closestMember, radius - bot->GetExactDist2d(closestMember));
        return MoveAway(closestMember, 5.0f);
    }

    return false;
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
