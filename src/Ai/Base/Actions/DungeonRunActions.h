/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DUNGEONRUNACTIONS_H
#define PLAYERBOTS_DUNGEONRUNACTIONS_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "AttackAction.h"
#include "ObjectGuid.h"

struct DungeonRoute;
struct DungeonRouteItem;

// Group leader out of combat on a map with a dungeon route: once the group is ready, walk the route node by node and
// pull the next pack or boss on it. Combat is left to the normal combat strategies; this only chooses where to go and
// what to pull next.
class DungeonRunAdvanceAction : public AttackAction
{
public:
    DungeonRunAdvanceAction(PlayerbotAI* botAI) : AttackAction(botAI, "dungeon run advance") {}

    bool Execute(Event event) override;
    bool isUseful() override;

private:
    static constexpr float PULL_DISTANCE = 25.0f;       // attack the pack from here
    static constexpr float PULL_APPROACH_MARGIN = 5.0f; // walking at a pack, stop this far inside pull range
    static constexpr float GROUP_RANGE = 35.0f;         // everyone this close before moving on
    static constexpr float READY_HEALTH_PCT = 60.0f;
    static constexpr float READY_MANA_PCT = 40.0f;
    static constexpr float STEP = 30.0f;                // how far ahead along the route the next waypoint is
    static constexpr float NODE_REACHED = 12.0f;        // counts as standing on a node
    static constexpr uint32 MAX_PULL_ATTEMPTS = 20;     // pulls issued on one pack before it is skipped
    static constexpr uint32 WAIT_LOG_INTERVAL_MS = 10000;
    static constexpr uint32 APPROACH_TIMEOUT_MS = 45000;
    static constexpr float PULL_BACK = 20.0f;             // pull position: the node this far behind
    static constexpr float PULL_FROM_SPOT_DISTANCE = 40.0f;  // from a pull spot, the pack is pulled at range
    static constexpr uint32 CC_WAIT_MS = 25000;           // longest wait for the crowd control to land
    static constexpr uint32 CC_NO_PLAN_MS = 10000;        // no crowd-control icon after this long: pull anyway
    static constexpr float OBJECT_SIGHT = 80.0f;          // closer than this, an object item is judged by its state
    static constexpr float SUMMON_SIGHT = 80.0f;          // closer than this, a summoned pack is judged by what stands
    static constexpr uint32 MAX_USE_ATTEMPTS = 5;         // uses of one object before it is skipped
    static constexpr uint32 SENT_WAIT_MS = 240000;        // longest wait for a pack the encounter sends; then pull it
    static constexpr float DROP_DEPTH = 50.0f;            // this far under a hole's rim counts as having dropped
    static constexpr float DROP_AT_RIM = 4.0f;            // this close to a drop's rim point, jump over the edge
    static constexpr float DROP_LEDGE_HEIGHT = 20.0f;     // a member this far above the leader below a drop is stuck
    static constexpr float DROP_LEDGE_RADIUS = 15.0f;     // ... when over the hole within this
    static constexpr float DROP_JUMP_HEIGHT = 2.0f;       // the jump lands this far above the hole's centre...
    static constexpr float DROP_JUMP_SPEED_XY = 7.0f;     // ...and gravity takes it from there
    static constexpr float DROP_JUMP_SPEED_Z = 8.0f;
    static constexpr float BOSS_AREA_MIN_RADIUS = 35.0f;  // packs between this far from a boss...
    static constexpr float BOSS_AREA_RADIUS = 70.0f;      // ...and this far are cleared before it...
    static constexpr float BOSS_AREA_AHEAD = 150.0f;      // ...when the route reaches them at most this far after it
    static constexpr uint8 TRASH_CC_SKULL_ICON = 7;
    static constexpr uint8 TRASH_CC_ICONS[] = { 3, 4, 5, 6 };  // triangle, moon, square, cross

    void UpdateProgress(DungeonRoute const& route);
    bool Pull(DungeonRoute const& route, DungeonRouteItem const& item, Unit* target, float progress);
    bool CcGateOpen(DungeonRouteItem const& item, Unit* nearest);
    bool UseObject(DungeonRouteItem const& item, uint32 index, GameObject* object);
    // Send the group's bots still above the hole to it, and over the rim once they stand at it; true if any is above.
    bool PushOverDrop(DungeonRouteItem const& item, float range);
    void StepOverDrop(DungeonRouteItem const& item);  // the leader itself, over the rim
    // An object item still to be used: in sight and usable (selectable, not yet activated), or too far away to tell.
    bool PendingObject(DungeonRouteItem const& item, GameObject*& object) const;
    bool ItemOpen(DungeonRoute const& route, uint32 index) const;  // on the path and not given up on
    bool BossAlive(DungeonRouteItem const& item) const;  // the boss creature alive, attackable or not
    Unit* NearestLivingMember(DungeonRouteItem const& item) const;  // attackable, nearest to the bot
    bool ApproachTimedOut(uint32 index, float distance);
    bool TraceDue();  // throttles the debug trace to one line per WAIT_LOG_INTERVAL_MS
    bool GroupReady(std::string& reason, Player*& dead, Player*& fighting) const;
    // First item on the path (not a side pack, not skipped) with a living, attackable member, or an object still to
    // be used; sets the member nearest to the bot, or the object when it is in sight.
    DungeonRouteItem const* NextItem(DungeonRoute const& route, Unit*& target, GameObject*& object,
                                     uint32& index) const;

    std::unordered_map<uint32, uint32> _pullAttempts;  // route item index -> pulls issued
    mutable std::unordered_set<uint32> _summonedDone;  // summoned packs seen cleared
    uint32 _lastWaitLogMs = 0;
    uint32 _approachItem = UINT32_MAX;  // item being walked to, best distance reached, since when
    float _approachBest = 0.0f;
    uint32 _approachSinceMs = 0;
    uint32 _lastApproachTickMs = 0;
    uint32 _lastTraceMs = 0;
    uint32 _dropItem = UINT32_MAX;  // hole the leader jumped down while members may still be above
    uint32 _sentItem = UINT32_MAX;  // sent pack being waited for, and since when
    uint32 _sentSinceMs = 0;
    ObjectGuid _ccGateTarget;
    uint32 _ccGateSinceMs = 0;
};

#endif
