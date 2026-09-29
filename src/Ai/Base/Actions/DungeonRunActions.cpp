/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "DungeonRunActions.h"

#include "DungeonRouteMgr.h"
#include "Playerbots.h"

bool DungeonRunAdvanceAction::isUseful()
{
    return !bot->IsInCombat() && bot->IsAlive() && DungeonRouteMgr::instance().Get(bot->GetMapId());
}

bool DungeonRunAdvanceAction::Execute(Event /*event*/)
{
    DungeonRoute const* route = DungeonRouteMgr::instance().Get(bot->GetMapId());
    if (!route)
        return false;

    UpdateProgress(*route);
    // Waiting yields the tick: eating, drinking, resurrecting and rebuffing are other actions.
    std::string waitReason;
    if (!GroupReady(waitReason))
    {
        uint32 const now = getMSTime();
        if (getMSTimeDiff(_lastWaitLogMs, now) >= WAIT_LOG_INTERVAL_MS)
        {
            _lastWaitLogMs = now;
            LOG_DEBUG("playerbots", "dungeon-run bot={} waiting: {} progress={:.0f}", bot->GetName(), waitReason,
                      AI_VALUE(float, "dungeon run progress"));
        }
        return false;
    }

    Unit* target = nullptr;
    uint32 index = 0;
    DungeonRouteItem const* item = NextItem(*route, target, index);
    if (!item || !target)
        return false;  // route cleared

    float const progress = AI_VALUE(float, "dungeon run progress");
    // Give up on a pack the leader cannot get closer to (unreachable ledge, no path): 45 s without gaining 2 yd.
    // Only continuous walking counts: a fight or a rest in between starts the clock again.
    uint32 const now = getMSTime();
    float const distance = bot->GetDistance(target);
    bool const resumed = getMSTimeDiff(_lastApproachTickMs, now) > 5000;
    _lastApproachTickMs = now;
    if (resumed || index != _approachItem || distance < _approachBest - 2.0f)
    {
        _approachItem = index;
        _approachBest = distance;
        _approachSinceMs = now;
    }
    else if (getMSTimeDiff(_approachSinceMs, now) >= APPROACH_TIMEOUT_MS)
    {
        _pullAttempts[index] = MAX_PULL_ATTEMPTS;
        LOG_DEBUG("playerbots", "dungeon-run bot={} skip item={} along={:.0f} target={} - no closer than {:.0f} yd",
                  bot->GetName(), index, item->along, target->GetName(), _approachBest);
        return false;
    }

    if (distance <= PULL_DISTANCE && bot->IsWithinLOSInMap(target))
    {
        uint32& attempts = _pullAttempts[index];
        ++attempts;
        LOG_DEBUG("playerbots", "dungeon-run bot={} pull item={} along={:.0f} target={} attempt={} progress={:.0f}",
                  bot->GetName(), index, item->along, target->GetName(), attempts, progress);
        return Attack(target);
    }

    // Walk the skeleton up to the pack: the next node about STEP yd ahead, never past the pack itself.
    float const goal = std::min(progress + STEP, item->along);
    DungeonRouteNode const* next = nullptr;
    for (DungeonRouteNode const& node : route->nodes)
    {
        if (node.along < goal)
            continue;
        next = &node;
        break;
    }
    // Past the last node before the pack, or the pack sits off the skeleton (a forge around a corner): walk to the
    // pack itself on the navmesh. MoveNear only picks points in sight and fails for a target behind a wall.
    if (!next || next->along > item->along || bot->GetDistance(next->x, next->y, next->z) < 3.0f)
        return MoveTo(bot->GetMapId(), target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), false,
                      false, false, false, MovementPriority::MOVEMENT_NORMAL);

    return MoveTo(bot->GetMapId(), next->x, next->y, next->z, false, false, false, false,
                  MovementPriority::MOVEMENT_NORMAL);
}

void DungeonRunAdvanceAction::UpdateProgress(DungeonRoute const& route)
{
    float progress = AI_VALUE(float, "dungeon run progress");
    // Only look near the current progress so a node on another floor straight above or below never counts.
    DungeonRouteNode const* best = nullptr;
    float bestDistance = NODE_REACHED;
    for (DungeonRouteNode const& node : route.nodes)
    {
        if (node.along < progress - 20.0f || node.along > progress + 60.0f)
            continue;
        float const distance = bot->GetDistance(node.x, node.y, node.z);
        if (distance < bestDistance)
        {
            best = &node;
            bestDistance = distance;
        }
    }
    if (best && best->along > progress)
        SET_AI_VALUE(float, "dungeon run progress", best->along);
}

bool DungeonRunAdvanceAction::GroupReady(std::string& reason) const
{
    Group* group = bot->GetGroup();
    if (!group)
        return true;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsInWorld() || member->GetMapId() != bot->GetMapId())
            continue;
        if (!member->IsAlive() || member->IsInCombat())
            reason = Acore::StringFormat("{} {}", member->GetName(), member->IsAlive() ? "in combat" : "dead");
        else if (member != bot && bot->GetDistance(member) > GROUP_RANGE)
            reason = Acore::StringFormat("{} {:.0f} yd away", member->GetName(), bot->GetDistance(member));
        else if (member->GetHealthPct() < READY_HEALTH_PCT)
            reason = Acore::StringFormat("{} health {:.0f}%", member->GetName(), member->GetHealthPct());
        else if (member->getPowerType() == POWER_MANA && member->GetMaxPower(POWER_MANA) > 0 &&
                 100.0f * member->GetPower(POWER_MANA) / member->GetMaxPower(POWER_MANA) < READY_MANA_PCT)
            reason = Acore::StringFormat("{} mana low", member->GetName());
        if (!reason.empty())
            return false;
    }
    return true;
}

DungeonRouteItem const* DungeonRunAdvanceAction::NextItem(DungeonRoute const& route, Unit*& target,
                                                          uint32& index) const
{
    Map* map = bot->GetMap();
    if (!map)
        return nullptr;

    for (uint32 i = 0; i < route.items.size(); ++i)
    {
        DungeonRouteItem const& item = route.items[i];
        if (item.side)
            continue;
        auto const attempts = _pullAttempts.find(i);
        if (attempts != _pullAttempts.end() && attempts->second >= MAX_PULL_ATTEMPTS)
            continue;

        Unit* nearest = nullptr;
        for (uint32 spawnId : item.spawnIds)
        {
            auto const bounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
            for (auto itr = bounds.first; itr != bounds.second; ++itr)
            {
                Creature* creature = itr->second;
                if (!creature || !creature->IsAlive() || !bot->IsValidAttackTarget(creature))
                    continue;
                if (!nearest || bot->GetDistance(creature) < bot->GetDistance(nearest))
                    nearest = creature;
            }
        }
        if (nearest)
        {
            target = nearest;
            index = i;
            return &item;
        }
    }
    return nullptr;
}
