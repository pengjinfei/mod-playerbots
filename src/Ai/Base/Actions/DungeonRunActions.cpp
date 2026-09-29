/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "DungeonRunActions.h"

#include "DungeonRouteMgr.h"
#include "PositionValue.h"
#include "PullStrategy.h"
#include "RtiTargetValue.h"
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
    // The pack the crowd-control gate pinned is done: stop signalling a pending pull to the trash-cc strategy.
    ObjectGuid const pinned = AI_VALUE(ObjectGuid, "pull target");
    if (pinned)
    {
        Unit* unit = botAI->GetUnit(pinned);
        if (!unit || !unit->IsAlive())
            context->GetValue<ObjectGuid>("pull target")->Set(ObjectGuid::Empty);
    }
    // Waiting yields the tick: eating, drinking, resurrecting and rebuffing are other actions.
    std::string waitReason;
    Player* dead = nullptr;
    if (!GroupReady(waitReason, dead))
    {
        // Take the group to a dead member so the healers are in range and in sight of the body: a rogue who died
        // on the ledge above the ramp lay 34 yd away and 11 yd up and was never resurrected (run 1810).
        // A member who released its spirit stands wherever its ghost is; the body is what gets resurrected.
        if (dead)
        {
            WorldLocation const body = dead->HasPlayerFlag(PLAYER_FLAGS_GHOST) ? dead->GetCorpseLocation()
                                                                                 : dead->GetWorldLocation();
            if (body.GetMapId() == bot->GetMapId() && bot->GetDistance(body) > 10.0f)
                MoveTo(bot->GetMapId(), body.GetPositionX(), body.GetPositionY(), body.GetPositionZ(), false, false,
                       false, false, MovementPriority::MOVEMENT_NORMAL);
        }
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
    {
        if (TraceDue())
            LOG_DEBUG("playerbots", "dungeon-run bot={} route cleared progress={:.0f}", bot->GetName(),
                      AI_VALUE(float, "dungeon run progress"));
        return false;
    }

    float const progress = AI_VALUE(float, "dungeon run progress");
    float const distance = bot->GetDistance(target);

    if (distance <= PULL_DISTANCE && bot->IsWithinLOSInMap(target))
    {
        // A pack of three or more gets the trash crowd-control chain first; the leader stands still until the
        // casters have it held (see CcGateOpen). One pull attempt is counted per gate, not per waiting tick.
        if (!CcGateOpen(*item, target))
            return true;
        Unit* pullUnit = TrashCcIconUnit(botAI, TRASH_CC_SKULL_ICON);
        if (!pullUnit || !pullUnit->IsAlive())
            pullUnit = target;
        uint32& attempts = _pullAttempts[index];
        ++attempts;
        LOG_DEBUG("playerbots", "dungeon-run bot={} pull item={} along={:.0f} target={} attempt={} progress={:.0f}",
                  bot->GetName(), index, item->along, pullUnit->GetName(), attempts, progress);
        return Pull(*route, pullUnit, progress);
    }

    // Walk the skeleton up to the pack: the next node about STEP yd ahead, never past the pack itself.
    float const goal = std::min(progress + STEP, item->along);
    DungeonRouteNode const* next = nullptr;
    for (DungeonRouteNode const& node : route->nodes)
    {
        if (node.along < goal)
            continue;
        // A node straight above or below (a stair landing over the leader's head) projects onto the leader's own
        // floor and the move goes nowhere (run 1784 stood under node 937 for 8 minutes); aim past it.
        if (bot->GetExactDist2d(node.x, node.y) < 8.0f && std::fabs(node.z - bot->GetPositionZ()) > 4.0f &&
            node.along < item->along)
            continue;
        next = &node;
        break;
    }
    // Past the last node before the pack, or the pack sits off the skeleton (a forge around a corner): walk to the
    // pack itself on the navmesh. MoveNear only picks points in sight and fails for a target behind a wall.
    bool const toTarget = !next || next->along > item->along || bot->GetDistance(next->x, next->y, next->z) < 3.0f;
    // Give up on a pack (never a boss) the leader cannot get closer to while walking straight at it - an unreachable
    // ledge: 45 s of continuous walking without gaining 2 yd. Walking the route nodes does not count; a leader that
    // cannot follow the route is a route problem and the stall watchdog reports it (run 1784 skipped Skarvald).
    if (toTarget && !item->boss && ApproachTimedOut(index, distance))
    {
        _pullAttempts[index] = MAX_PULL_ATTEMPTS;
        LOG_DEBUG("playerbots", "dungeon-run bot={} skip item={} along={:.0f} target={} - no closer than {:.0f} yd",
                  bot->GetName(), index, item->along, target->GetName(), _approachBest);
        return false;
    }
    bool const moved = toTarget ? MoveTo(bot->GetMapId(), target->GetPositionX(), target->GetPositionY(),
                                         target->GetPositionZ(), false, false, false, false,
                                         MovementPriority::MOVEMENT_NORMAL)
                                : MoveTo(bot->GetMapId(), next->x, next->y, next->z, false, false, false, false,
                                         MovementPriority::MOVEMENT_NORMAL);
    if (TraceDue())
        LOG_DEBUG("playerbots", "dungeon-run bot={} approach item={} target={} dist={:.1f} los={} to={} moved={} "
                  "progress={:.0f}", bot->GetName(), index, target->GetName(), distance, bot->IsWithinLOSInMap(target),
                  toTarget ? "target" : "node", moved, progress);
    return moved;
}

// Pull the way the "pull" strategy does it on a chat command: a ranged pull, then back to the pull position so the
// pack comes to the group. The pull position is the route node PULL_BACK yd behind, away from the packs ahead: a
// melee pull where the leader stood brought the neighbouring forge packs along (run 1787, six elites, wipe).
bool DungeonRunAdvanceAction::Pull(DungeonRoute const& route, Unit* target, float progress)
{
    PullStrategy* strategy = PullStrategy::Get(botAI);
    if (!strategy || strategy->HasPullStarted() || !strategy->CanDoPullAction(target))
        return Attack(target);

    DungeonRouteNode const* back = nullptr;
    for (DungeonRouteNode const& node : route.nodes)
    {
        if (node.along > progress - PULL_BACK)
            break;
        back = &node;
    }
    PositionMap& positions = AI_VALUE(PositionMap&, "position");
    PositionInfo pullPosition = positions["pull"];
    if (back)
        pullPosition.Set(back->x, back->y, back->z, bot->GetMapId());
    else
        pullPosition.Set(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetMapId());
    positions["pull"] = pullPosition;

    strategy->RequestPull(target);
    context->GetValue<Unit*>("current target")->Set(target);
    botAI->ChangeEngine(BOT_STATE_COMBAT);
    botAI->SetNextCheckDelay(sPlayerbotAIConfig.reactDelay);
    return true;
}

// Crowd control before the pull, the leader's side of the chain the test harness used to run (AttemptRunner
// CcPullGateReady): pin "pull target" on the pack - the trash-cc strategy of the dungeon marks it and the mage,
// shaman and rogue cast on their icons - then open when every crowd-control icon is held, when something loose in
// the pack is already fighting, or when the wait runs out. Two elite packs pulled together wiped the group on
// Utgarde Keep's upper floor (run 1798).
bool DungeonRunAdvanceAction::CcGateOpen(DungeonRouteItem const& item, Unit* nearest)
{
    // The trash-cc chain sees the pack as what stands within 15 yd of the pinned unit; pin the member nearest the
    // pack's centre, not the one nearest the leader, or a spread pack (four elites over 18 yd) never counts as one.
    Unit* target = nearest;
    float best = nearest->GetExactDist(item.x, item.y, item.z);
    if (Map* map = bot->GetMap())
        for (uint32 spawnId : item.spawnIds)
        {
            auto const bounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
            for (auto itr = bounds.first; itr != bounds.second; ++itr)
            {
                Creature* creature = itr->second;
                if (!creature || !creature->IsAlive() || !bot->IsValidAttackTarget(creature))
                    continue;
                float const distance = creature->GetExactDist(item.x, item.y, item.z);
                if (distance < best)
                {
                    best = distance;
                    target = creature;
                }
            }
        }
    std::vector<Creature*> const pack = TrashCcCollectPack(botAI, bot, target);
    if (pack.empty())
    {
        LOG_DEBUG("playerbots", "dungeon-run bot={} cc gate skipped: fewer than three around {}", bot->GetName(),
                  target->GetName());
        return true;  // fewer than three: nothing to control
    }

    uint32 const now = getMSTime();
    if (_ccGateTarget != target->GetGUID())
    {
        _ccGateTarget = target->GetGUID();
        _ccGateSinceMs = now;
        context->GetValue<ObjectGuid>("pull target")->Set(target->GetGUID());
        return false;
    }

    uint32 icons = 0;
    uint32 held = 0;
    for (uint8 icon : TRASH_CC_ICONS)
        if (Unit* unit = TrashCcIconUnit(botAI, icon))
        {
            ++icons;
            if (TrashCcIncapacitated(unit, bot))
                ++held;
        }
    bool engaged = false;
    for (Creature* creature : pack)
        if (creature->IsAlive() && creature->IsInCombat() && !TrashCcIncapacitated(creature, bot))
            engaged = true;

    uint32 const waited = getMSTimeDiff(_ccGateSinceMs, now);
    char const* reason = nullptr;
    if (icons && held == icons)
        reason = "cc_ready";
    else if (engaged)
        reason = "pack_engaged";
    else if (!icons && waited >= CC_NO_PLAN_MS)
        reason = "no_plan";
    else if (waited >= CC_WAIT_MS)
        reason = "timeout";
    if (!reason)
        return false;
    LOG_DEBUG("playerbots", "dungeon-run bot={} cc gate open: {} icons={} held={} waited={}ms", bot->GetName(),
              reason, icons, held, waited);
    return true;
}

bool DungeonRunAdvanceAction::ApproachTimedOut(uint32 index, float distance)
{
    // Only continuous walking counts: a fight or a rest in between starts the clock again.
    uint32 const now = getMSTime();
    bool const resumed = getMSTimeDiff(_lastApproachTickMs, now) > 5000;
    _lastApproachTickMs = now;
    if (resumed || index != _approachItem || distance < _approachBest - 2.0f)
    {
        _approachItem = index;
        _approachBest = distance;
        _approachSinceMs = now;
        return false;
    }
    return getMSTimeDiff(_approachSinceMs, now) >= APPROACH_TIMEOUT_MS;
}

bool DungeonRunAdvanceAction::TraceDue()
{
    uint32 const now = getMSTime();
    if (getMSTimeDiff(_lastTraceMs, now) < WAIT_LOG_INTERVAL_MS)
        return false;
    _lastTraceMs = now;
    return true;
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

bool DungeonRunAdvanceAction::GroupReady(std::string& reason, Player*& dead) const
{
    Group* group = bot->GetGroup();
    if (!group)
        return true;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsInWorld() || member->GetMapId() != bot->GetMapId())
            continue;
        // "In combat" only counts while something actually attacks the member: a combat flag that lingers with no
        // attacker kept the leader waiting for five minutes on Utgarde Keep's stairs (run 1806).
        if (!member->IsAlive() || (member->IsInCombat() && !member->getAttackers().empty()))
        {
            reason = Acore::StringFormat("{} {}", member->GetName(), member->IsAlive() ? "in combat" : "dead");
            if (!member->IsAlive())
                dead = member;
        }
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
