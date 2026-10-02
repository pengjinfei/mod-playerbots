/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "DungeonRunActions.h"

#include <algorithm>
#include <cmath>

#include "DungeonRouteMgr.h"
#include "LastMovementValue.h"
#include "PositionValue.h"
#include "PullStrategy.h"
#include "RtiTargetValue.h"
#include "Playerbots.h"

bool DungeonRunAdvanceAction::isUseful()
{
    // A combat flag with nobody attacking does not hold the leader: Hadronox's gauntlet left the tank flagged in
    // combat for minutes with no attacker, and the run stood still on the ramp (run 1857).
    return bot->IsAlive() && (!bot->IsInCombat() || bot->getAttackers().empty()) &&
           DungeonRouteMgr::instance().Get(bot->GetMapId());
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
    // Down a hole ahead of the group: bring the rest down before anything else (they are out of range above).
    if (_dropItem < route->items.size())
    {
        DungeonRouteItem const& drop = route->items[_dropItem];
        // Still above: the step over was cut short (run 1865: the leader stayed on the ledge while it pushed the
        // others). Step over again.
        if (bot->GetPositionZ() > drop.z - DROP_DEPTH)
        {
            PushOverDrop(drop, GROUP_RANGE);
            StepOverDrop(drop);
            return true;
        }
        if (PushOverDrop(drop, 1000.0f))
            return false;
        _dropItem = UINT32_MAX;
    }
    // Waiting yields the tick: eating, drinking, resurrecting and rebuffing are other actions.
    std::string waitReason;
    Player* dead = nullptr;
    Player* fighting = nullptr;
    if (!GroupReady(waitReason, dead, fighting))
    {
        // Go to a member something is fighting out of the leader's sight: the Azure Magus caught the priest 16 yd off
        // the entrance corridor while the others had walked on 40 yd round the corner, and the leader waited there
        // for eight minutes while the priest fought him alone (run 1936).
        if (!dead && fighting && fighting != bot &&
            (bot->GetDistance(fighting) > 10.0f || !bot->IsWithinLOSInMap(fighting)))
            MoveTo(fighting->GetMapId(), fighting->GetPositionX(), fighting->GetPositionY(), fighting->GetPositionZ(),
                   false, false, false, false, MovementPriority::MOVEMENT_NORMAL);
        // Take the group to a dead member so the healers are in range and in sight of the body: a rogue who died
        // on the ledge above the ramp lay 34 yd away and 11 yd up and was never resurrected (run 1810).
        // A member who released its spirit stands wherever its ghost is; the body is what gets resurrected.
        if (dead)
        {
            WorldLocation const body = dead->HasPlayerFlag(PLAYER_FLAGS_GHOST) ? dead->GetCorpseLocation()
                                                                                 : dead->GetWorldLocation();
            if (body.GetMapId() == bot->GetMapId() && bot->GetDistance(body) > 10.0f)
            {
                bool const moved = MoveTo(bot->GetMapId(), body.GetPositionX(), body.GetPositionY(),
                                          body.GetPositionZ(), false, false, false, false,
                                          MovementPriority::MOVEMENT_NORMAL);
                if (TraceDue())
                {
                    LastMovement& lastMove = *context->GetValue<LastMovement&>("last movement");
                    LOG_DEBUG("playerbots", "dungeon-run bot={} to body of {} dist={:.1f} moved={} can_move={} "
                              "last_priority={} last_age_ms={}", bot->GetName(), dead->GetName(),
                              bot->GetDistance(body), moved, botAI->CanMove(), uint32(lastMove.priority),
                              getMSTimeDiff(lastMove.msTime, getMSTime()));
                }
            }
        }
        uint32 const now = getMSTime();
        if (getMSTimeDiff(_lastWaitLogMs, now) >= WAIT_LOG_INTERVAL_MS)
        {
            _lastWaitLogMs = now;
            LOG_DEBUG("playerbots", "dungeon-run bot={} waiting: {} progress={:.0f}", bot->GetName(), waitReason,
                      AI_VALUE(float, "dungeon run progress"));
            if (dead)
                LOG_DEBUG("playerbots", "dungeon-run bot={} dead member {} death_state={} ghost={} rez_requested={} "
                          "dist={:.1f}", bot->GetName(), dead->GetName(), uint32(dead->getDeathState()),
                          dead->HasPlayerFlag(PLAYER_FLAGS_GHOST), dead->isResurrectRequested(),
                          bot->GetDistance(dead));
        }
        return false;
    }

    Unit* target = nullptr;
    GameObject* object = nullptr;
    uint32 index = 0;
    DungeonRouteItem const* item = NextItem(*route, target, object, index);
    // The boss is there but cannot be attacked yet: wait for it where the route says.
    if (item && item->boss && !target)
    {
        if (item->hold && bot->GetExactDist(item->holdX, item->holdY, item->holdZ) > 5.0f)
            return MoveTo(bot->GetMapId(), item->holdX, item->holdY, item->holdZ, false, false, false, false,
                          MovementPriority::MOVEMENT_NORMAL);
        if (TraceDue())
            LOG_DEBUG("playerbots", "dungeon-run bot={} waiting for boss item={} to be attackable", bot->GetName(),
                      index);
        return false;
    }
    if (!item || (!target && !item->object && !item->drop && item->summonEntries.empty()))
    {
        if (TraceDue())
            LOG_DEBUG("playerbots", "dungeon-run bot={} route cleared progress={:.0f}", bot->GetName(),
                      AI_VALUE(float, "dungeon run progress"));
        return false;
    }

    float const progress = AI_VALUE(float, "dungeon run progress");
    if (object && bot->IsWithinDistInMap(object, object->GetInteractionDistance() - 1.0f))
        return UseObject(*item, index, object);

    // A hole to jump down (Azjol-Nerub: from Hadronox's pit into the pool of Anub'arak's cavern, 360 yd below; the
    // water takes the fall as it does for a player). Walk to the rim, then everyone steps over; gravity does the rest.
    if (item->drop)
    {
        // Walk to the rim on the navmesh; a path aimed at the hole itself ends wherever the navmesh has a floor
        // under it - z 0 here - and took the tank straight down through the level (run 1867).
        if (bot->GetExactDist(item->rimX, item->rimY, item->rimZ) > DROP_AT_RIM)
            return MoveTo(bot->GetMapId(), item->rimX, item->rimY, item->rimZ, false, false, false, false,
                          MovementPriority::MOVEMENT_NORMAL);
        _dropItem = index;
        PushOverDrop(*item, GROUP_RANGE);
        StepOverDrop(*item);
        LOG_DEBUG("playerbots", "dungeon-run bot={} drop item={} along={:.0f} at ({:.1f},{:.1f},{:.1f})",
                  bot->GetName(), index, item->along, item->x, item->y, item->z);
        return true;
    }

    // A pack the encounter sends by itself: stand and wait for it. Krik'thir sends his watchers one at a time once
    // the first is engaged; the leader pulling the next one as well brought two groups at once and wiped (run 1856).
    if (item->sent)
    {
        // Wait where the route says: Hadronox has to be met on the upper platform, where she webs the tunnel doors
        // shut; met halfway down the ramp she kept eating the crypt fiends that poured out and never died (run 1868).
        if (item->hold && bot->GetExactDist(item->holdX, item->holdY, item->holdZ) > 5.0f)
            return MoveTo(bot->GetMapId(), item->holdX, item->holdY, item->holdZ, false, false, false, false,
                          MovementPriority::MOVEMENT_NORMAL);
        uint32 const now = getMSTime();
        if (_sentItem != index)
        {
            _sentItem = index;
            _sentSinceMs = now;
        }
        if (getMSTimeDiff(_sentSinceMs, now) < SENT_WAIT_MS)
        {
            if (TraceDue())
                LOG_DEBUG("playerbots", "dungeon-run bot={} holding for sent item={} target={} waited={}ms",
                          bot->GetName(), index, target ? target->GetName() : "-",
                          getMSTimeDiff(_sentSinceMs, now));
            return false;
        }
    }

    // A pack with a pull spot is pulled from there, as a tank stops in a doorway: walking the route into the room
    // took the leader from first sight of Kolurg's escort (36 yd) to 20 yd of it in one step, and Kolurg came too
    // (run 1923). Away from the spot, nothing else is pulled on the way.
    bool atSpot = false;
    if (target && item->from)
    {
        if (bot->GetExactDist(item->fromX, item->fromY, item->fromZ) > 3.0f)
            return MoveTo(bot->GetMapId(), item->fromX, item->fromY, item->fromZ, false, false, false, false,
                          MovementPriority::MOVEMENT_NORMAL);
        atSpot = true;
    }
    // A pack whose aggro reaches past the default pull distance is engaged from farther, before anyone walks into
    // it: Krik'thir's first watcher group aggroed while the leader was still closing in, before any crowd control.
    // At a spot the route's own pull distance still holds: a patrol is pulled only at the end of its walk, away from
    // the squad it passes (Ahn'kahet's lower room, run 1969).
    float const pullDistance = item->pullDistance > 0.0f ? item->pullDistance
                               : atSpot                  ? PULL_FROM_SPOT_DISTANCE
                                                         : PULL_DISTANCE;
    // At the spot with the pack out of reach: wait there, as players wait in a doorway for a patrol to come by. Walking
    // after it took the tank past the next squad (Ahn'kahet's first hall, run 1965), and a step off the spot sent the
    // leader straight back to it. A pack that never comes is walked to after a while.
    if (atSpot && !(bot->GetDistance(target) <= pullDistance && bot->IsWithinLOSInMap(target)))
    {
        uint32 const now = getMSTime();
        if (_spotItem != index)
        {
            _spotItem = index;
            _spotSinceMs = now;
        }
        if (getMSTimeDiff(_spotSinceMs, now) < SPOT_WAIT_MS)
        {
            if (TraceDue())
                LOG_DEBUG("playerbots", "dungeon-run bot={} waiting at the pull spot of item={} for {} dist={:.1f}",
                          bot->GetName(), index, target->GetName(), bot->GetDistance(target));
            return false;
        }
    }
    if (target && bot->GetDistance(target) <= pullDistance && bot->IsWithinLOSInMap(target))
    {
        // A pack of three or more gets the trash crowd-control chain first; the leader stands still until the
        // casters have it held (see CcGateOpen). One pull attempt is counted per gate, not per waiting tick.
        // Not where the sapper is seen first: the rogue walking up to Kolurg's escort was hit at 7 yd and the whole
        // group came while the tank stood at the doorway spot, so nothing was pulled back round the bend (run 1929).
        if (!item->noCc && !CcGateOpen(*item, target))
            return true;
        // Off the mount and pull in the same tick: the outdoor stretch before Ingvar lets the bots ride, and every
        // pull spell failed with "not mounted" until the boss was skipped as unpullable (normal, run 1898); a
        // dismount that yielded the tick was undone by "check mount state" every time (run 1900).
        if (bot->IsMounted())
        {
            WorldPacket packet;
            bot->GetSession()->HandleCancelMountAuraOpcode(packet);
            bot->RemoveAurasByType(SPELL_AURA_MOUNTED);
        }
        Unit* pullUnit = TrashCcIconUnit(botAI, TRASH_CC_SKULL_ICON);
        if (!pullUnit || !pullUnit->IsAlive())
            pullUnit = target;
        uint32& attempts = _pullAttempts[index];
        ++attempts;
        LOG_DEBUG("playerbots", "dungeon-run bot={} pull item={} along={:.0f} target={} attempt={} progress={:.0f}",
                  bot->GetName(), index, item->along, pullUnit->GetName(), attempts, progress);
        return Pull(*route, *item, pullUnit, progress);
    }

    // Where the item is: the pack member, the object, or - an object not loaded yet - the route's position for it.
    Position const destination = target ? target->GetPosition()
                                 : object ? object->GetPosition()
                                          : Position(item->x, item->y, item->z);
    std::string const name = target ? target->GetName() : object ? object->GetName() : "object";
    float const distance = target ? bot->GetDistance(target) : bot->GetExactDist(destination);

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
    // A node already inside pull range of the pack is walked at as the pack itself, so the stop short below holds:
    // node 630 lies 16 yd from Telestra's first mage-hunter group and the leader walked into it (run 1927).
    bool const toTarget = !next || next->along > item->along || bot->GetDistance(next->x, next->y, next->z) < 3.0f ||
                          (target && target->GetExactDist(next->x, next->y, next->z) < pullDistance);
    // Give up on a pack (never a boss) the leader cannot get closer to while walking straight at it - an unreachable
    // ledge: 45 s of continuous walking without gaining 2 yd. Walking the route nodes does not count; a leader that
    // cannot follow the route is a route problem and the stall watchdog reports it (run 1784 skipped Skarvald).
    if (toTarget && !item->boss && ApproachTimedOut(index, distance))
    {
        _pullAttempts[index] = MAX_PULL_ATTEMPTS;
        LOG_DEBUG("playerbots", "dungeon-run bot={} skip item={} along={:.0f} target={} - no closer than {:.0f} yd",
                  bot->GetName(), index, item->along, name, _approachBest);
        return false;
    }
    // Straight at a pack, stop short of it and pull on the next tick: walking at the pack member itself took the
    // leader from 36 yd to 15 yd of the mage hunters past Telestra within one tick, inside their aggro (run 1925).
    // Out of sight there, the next tick walks on at the member.
    Position approach = destination;
    if (toTarget && target && distance > pullDistance)
    {
        float const angle = target->GetAngle(bot);
        float const shortOf = pullDistance - PULL_APPROACH_MARGIN;
        float x = target->GetPositionX() + shortOf * std::cos(angle);
        float y = target->GetPositionY() + shortOf * std::sin(angle);
        // Short of every member, not only the one walked at: a spread pack's steward stood 5 yd nearer the leader,
        // 15 yd from the stop, and pulled the pack before the leader did (run 1939).
        float deficit = 0.0f;
        if (Map* map = bot->GetMap())
            for (uint32 spawnId : item->spawnIds)
            {
                auto const bounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
                for (auto itr = bounds.first; itr != bounds.second; ++itr)
                    if (itr->second && itr->second->IsAlive())
                        deficit = std::max(deficit, shortOf - itr->second->GetExactDist2d(x, y));
            }
        x += deficit * std::cos(angle);
        y += deficit * std::sin(angle);
        // The target's height, not a ground search: where the collision model has no floor (Ahn'kahet's ledges) the
        // search found the void under the map and the tank walked straight down through the ledge (run 1968). The
        // path search puts the point on the navmesh.
        approach.Relocate(x, y, target->GetPositionZ());
    }
    bool const moved = toTarget ? MoveTo(bot->GetMapId(), approach.GetPositionX(), approach.GetPositionY(),
                                         approach.GetPositionZ(), false, false, false, false,
                                         MovementPriority::MOVEMENT_NORMAL)
                                : MoveTo(bot->GetMapId(), next->x, next->y, next->z, false, false, false, false,
                                         MovementPriority::MOVEMENT_NORMAL);
    if (TraceDue())
        LOG_DEBUG("playerbots", "dungeon-run bot={} approach item={} target={} dist={:.1f} los={} to={} moved={} "
                  "progress={:.0f}", bot->GetName(), index, name, distance,
                  target ? bot->IsWithinLOSInMap(target) : true, toTarget ? "target" : "node", moved, progress);
    return moved;
}

// Use an object on the route the way a client click does. The Nexus' containment spheres must be used after their
// bosses die before Keristrasza leaves her prison; bots never click objects on their own.
bool DungeonRunAdvanceAction::UseObject(DungeonRouteItem const& item, uint32 index, GameObject* object)
{
    uint32& attempts = _pullAttempts[index];
    if (++attempts > MAX_USE_ATTEMPTS)
    {
        attempts = MAX_PULL_ATTEMPTS;
        LOG_DEBUG("playerbots", "dungeon-run bot={} skip item={} along={:.0f} object={} - still usable after {} uses",
                  bot->GetName(), index, item.along, object->GetName(), MAX_USE_ATTEMPTS);
        return false;
    }
    LOG_DEBUG("playerbots", "dungeon-run bot={} use item={} along={:.0f} object={} attempt={}", bot->GetName(), index,
              item.along, object->GetName(), attempts);
    WorldPacket packet(CMSG_GAMEOBJ_USE);
    packet << object->GetGUID();
    bot->GetSession()->HandleGameObjectUseOpcode(packet);
    return true;
}

void DungeonRunAdvanceAction::StepOverDrop(DungeonRouteItem const& item)
{
    if (!bot->movespline->Finalized())
        return;
    if (bot->GetExactDist(item.rimX, item.rimY, item.rimZ) > DROP_AT_RIM)
        MoveTo(bot->GetMapId(), item.rimX, item.rimY, item.rimZ, false, false, false, false,
               MovementPriority::MOVEMENT_NORMAL);
    else
        // Jump, as a player does: a straight walk to the hole stopped at the web's edge for all five (run 1875).
        bot->GetMotionMaster()->MoveJump(item.x, item.y, item.z + DROP_JUMP_HEIGHT, DROP_JUMP_SPEED_XY,
                                         DROP_JUMP_SPEED_Z);
}

bool DungeonRunAdvanceAction::PushOverDrop(DungeonRouteItem const& item, float range)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;
    bool above = false;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive() || member->GetMapId() != bot->GetMapId() ||
            !GET_PLAYERBOT_AI(member))
            continue;
        if (member->GetPositionZ() < item.z - DROP_DEPTH)
        {
            // Caught on a web strand partway down, 2 yd off the line the others fell (run 1958: a member stood at
            // z 380 over the pool at 288 and the leader waited for it until the run stalled). Once the leader is
            // down, a member well above it over the hole steps sideways onto that line and falls the rest.
            bool const leaderDown = bot->GetPositionZ() < item.z - DROP_DEPTH;
            if (!leaderDown || member->GetPositionZ() < bot->GetPositionZ() + DROP_LEDGE_HEIGHT ||
                member->GetExactDist2d(item.x, item.y) > DROP_LEDGE_RADIUS)
                continue;
            above = true;
            if (member->movespline->Finalized())
                member->GetMotionMaster()->MoveJump(item.x, item.y, member->GetPositionZ() + 1.0f, DROP_JUMP_SPEED_XY,
                                                    DROP_JUMP_SPEED_Z);
            continue;
        }
        above = true;
        // Mid-jump or mid-walk to the rim: let it finish; a new order every tick restarted the jump.
        if (member->GetExactDist2d(item.x, item.y) > range || !member->movespline->Finalized())
            continue;
        // At the rim: straight over it, no path (there is none). Farther: walk to the rim first. In the controlled
        // slot, so the member's own follow movement does not replace it (only one of five went over in run 1865).
        if (member->GetExactDist(item.rimX, item.rimY, item.rimZ) > DROP_AT_RIM)
            member->GetMotionMaster()->MovePoint(0, item.rimX, item.rimY, item.rimZ, FORCED_MOVEMENT_NONE, 0.0f, 0.0f,
                                                 true, true, MOTION_SLOT_CONTROLLED);
        else
            member->GetMotionMaster()->MoveJump(item.x, item.y, item.z + DROP_JUMP_HEIGHT, DROP_JUMP_SPEED_XY,
                                                DROP_JUMP_SPEED_Z);
    }
    if (above && TraceDue())
        LOG_DEBUG("playerbots", "dungeon-run bot={} bringing the group down the hole at ({:.1f},{:.1f})",
                  bot->GetName(), item.x, item.y);
    return above;
}

bool DungeonRunAdvanceAction::PendingObject(DungeonRouteItem const& item, GameObject*& object) const
{
    object = nullptr;
    // Beyond this the object's grid may not be loaded: walk there and judge it on arrival.
    if (bot->GetExactDist(item.x, item.y, item.z) > OBJECT_SIGHT)
        return true;
    std::list<GameObject*> objects;
    bot->GetGameObjectListWithEntryInGrid(objects, item.objectEntries, OBJECT_SIGHT + 20.0f);
    for (GameObject* candidate : objects)
    {
        if (candidate->GetExactDist(item.x, item.y, item.z) > 10.0f || !candidate->isSpawned())
            continue;
        // Not selectable: its condition is unmet (a sphere whose boss lives, when that boss was skipped) - pass on.
        // Activated: already used.
        if (!candidate->HasGameObjectFlag(GO_FLAG_NOT_SELECTABLE) && candidate->GetGoState() == GO_STATE_READY)
        {
            object = candidate;
            return true;
        }
    }
    return false;
}

// Pull the way the "pull" strategy does it on a chat command: a ranged pull, then back to the pull position so the
// pack comes to the group. The pull position is the route node PULL_BACK yd behind, away from the packs ahead: a
// melee pull where the leader stood brought the neighbouring forge packs along (run 1787, six elites, wipe).
bool DungeonRunAdvanceAction::Pull(DungeonRoute const& route, DungeonRouteItem const& item, Unit* target,
                                   float progress)
{
    PullStrategy* strategy = PullStrategy::Get(botAI);
    if (!strategy || strategy->HasPullStarted() || !strategy->CanDoPullAction(target))
        return Attack(target);

    // Only a node on the leader's floor: below a drop the nodes behind are 360 yd up, and a pull back to one sent
    // the tank down a path ending at z 0, through the level (run 1878).
    DungeonRouteNode const* back = nullptr;
    for (DungeonRouteNode const& node : route.nodes)
    {
        if (node.along > progress - PULL_BACK)
            break;
        if (std::fabs(node.z - bot->GetPositionZ()) <= 10.0f)
            back = &node;
    }
    PositionMap& positions = AI_VALUE(PositionMap&, "position");
    PositionInfo pullPosition = positions["pull"];
    // A pack with a hold point is fought there, as players do: Krik'thir's watchers are pulled back up the entrance
    // ramp, round the corner, so the casters have to come into melee and the groups he sends arrive one at a time.
    if (item.hold)
        pullPosition.Set(item.holdX, item.holdY, item.holdZ, bot->GetMapId());
    else if (back)
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

bool DungeonRunAdvanceAction::GroupReady(std::string& reason, Player*& dead, Player*& fighting) const
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
            else
                fighting = member;
        }
        // Sitting to eat or drink: a healer still drinking at 72% mana was left 36 yd behind and the tank fought the
        // next pack alone (run 1927).
        else if (member->IsSitState() &&
                 (member->HasAuraType(SPELL_AURA_MOD_POWER_REGEN) || member->HasAuraType(SPELL_AURA_MOD_REGEN)))
            reason = Acore::StringFormat("{} eating or drinking", member->GetName());
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
                                                          GameObject*& object, uint32& index) const
{
    Map* map = bot->GetMap();
    if (!map)
        return nullptr;

    for (uint32 i = 0; i < route.items.size(); ++i)
    {
        DungeonRouteItem const& item = route.items[i];
        if (!ItemOpen(route, i))
            continue;
        if (item.object)
        {
            // Once seen used it stays done: out of sight again it read as "not loaded yet, walk there" and the leader
            // walked back to Taldaram's first device four times from the hall below (Ahn'kahet, run 1970).
            if (_objectsDone.count(i))
                continue;
            if (!PendingObject(item, object))
            {
                if (bot->GetExactDist(item.x, item.y, item.z) <= OBJECT_SIGHT)
                    _objectsDone.insert(i);
                continue;
            }
            index = i;
            return &item;
        }
        if (item.drop)
        {
            if (bot->GetPositionZ() < item.z - DROP_DEPTH)
                continue;  // already down
            target = nullptr;
            index = i;
            return &item;
        }

        // A pack a script summons has no spawn ids; beyond grid-search range it cannot be seen yet: walk there. Once
        // seen cleared it stays done - out of sight again it read as not yet seen and the leader walked back up
        // from Hadronox's pit to the crusher pack it had killed (run 1873).
        if (!item.summonEntries.empty() && _summonedDone.count(i))
            continue;
        if (!item.summonEntries.empty() && bot->GetExactDist(item.x, item.y, item.z) > SUMMON_SIGHT)
        {
            target = nullptr;
            index = i;
            return &item;
        }
        Unit* nearest = NearestLivingMember(item);
        if (!nearest)
        {
            // A boss alive but out of reach for now (evading home, resetting) is not cleared: Hadronox evaded, the
            // next item was the hole in her pit, and the leader walked down into the tunnel her adds pour out of
            // (run 1955).
            if (item.boss && BossAlive(item))
            {
                target = nullptr;
                index = i;
                return &item;
            }
            if (!item.summonEntries.empty())
                _summonedDone.insert(i);
            continue;
        }
        // Clear what stands around a boss before pulling it, even when the route reaches it only after the boss: a
        // boss fight spreads over its room, and a pack 60 yd past Telestra joined her split phase and wiped the
        // group (run 1819).
        if (item.boss)
            for (uint32 j = i + 1; j < route.items.size(); ++j)
            {
                DungeonRouteItem const& after = route.items[j];
                if (after.along > item.along + BOSS_AREA_AHEAD)
                    break;
                // Not a pack right beside the boss: reaching it means walking past the boss, and it joins the boss
                // fight anyway (Kolurg's berserkers 21 yd behind him: the leader walked into the boss, wipe, run 1829).
                float const fromBoss = std::hypot(after.x - item.x, after.y - item.y);
                if (after.boss || after.object || !ItemOpen(route, j) || fromBoss < BOSS_AREA_MIN_RADIUS ||
                    fromBoss > BOSS_AREA_RADIUS || std::fabs(after.z - item.z) > 10.0f)
                    continue;
                if (Unit* member = NearestLivingMember(after))
                {
                    target = member;
                    index = j;
                    return &after;
                }
            }
        target = nearest;
        index = i;
        return &item;
    }
    return nullptr;
}

bool DungeonRunAdvanceAction::ItemOpen(DungeonRoute const& route, uint32 index) const
{
    if (route.items[index].side)
        return false;
    auto const attempts = _pullAttempts.find(index);
    return attempts == _pullAttempts.end() || attempts->second < MAX_PULL_ATTEMPTS;
}

bool DungeonRunAdvanceAction::BossAlive(DungeonRouteItem const& item) const
{
    Map* map = bot->GetMap();
    if (!map)
        return false;
    for (uint32 spawnId : item.spawnIds)
    {
        auto const bounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
        for (auto itr = bounds.first; itr != bounds.second; ++itr)
        {
            Creature* creature = itr->second;
            if (!creature || !creature->IsAlive())
                continue;
            // The boss itself, not whoever stands with it: Kolurg's room has an Alliance Commander no one fights.
            // The spawn's own entry counts too, for a boss swapped by faction (Stoutbeard's spawn is Kolurg's).
            CreatureData const* data = creature->GetCreatureData();
            for (uint32 entry : item.bossEntries)
                if (creature->GetEntry() == entry || (data && data->id == entry))
                    return true;
        }
    }
    return false;
}

Unit* DungeonRunAdvanceAction::NearestLivingMember(DungeonRouteItem const& item) const
{
    Map* map = bot->GetMap();
    Unit* nearest = nullptr;
    if (!item.summonEntries.empty())
    {
        std::list<Creature*> creatures;
        bot->GetCreatureListWithEntryInGrid(creatures, item.summonEntries, SUMMON_SIGHT + item.radius);
        for (Creature* creature : creatures)
        {
            if (!creature->IsAlive() || !bot->IsValidAttackTarget(creature) ||
                creature->GetExactDist(item.x, item.y, item.z) > item.radius)
                continue;
            if (!nearest || bot->GetDistance(creature) < bot->GetDistance(nearest))
                nearest = creature;
        }
        return nearest;
    }
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
    return nearest;
}
