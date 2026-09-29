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
    if (!GroupReady())
        return false;

    Unit* target = nullptr;
    uint32 index = 0;
    DungeonRouteItem const* item = NextItem(*route, target, index);
    if (!item || !target)
        return false;  // route cleared

    float const progress = AI_VALUE(float, "dungeon run progress");
    if (bot->GetDistance(target) <= PULL_DISTANCE && bot->IsWithinLOSInMap(target))
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
    if (!next || next->along > item->along || bot->GetDistance(next->x, next->y, next->z) < 3.0f)
        return MoveNear(target, PULL_DISTANCE - 5.0f, MovementPriority::MOVEMENT_NORMAL);

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

bool DungeonRunAdvanceAction::GroupReady() const
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
            return false;
        if (member != bot && bot->GetDistance(member) > GROUP_RANGE)
            return false;
        if (member->GetHealthPct() < READY_HEALTH_PCT)
            return false;
        if (member->getPowerType() == POWER_MANA && member->GetMaxPower(POWER_MANA) > 0 &&
            100.0f * member->GetPower(POWER_MANA) / member->GetMaxPower(POWER_MANA) < READY_MANA_PCT)
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
