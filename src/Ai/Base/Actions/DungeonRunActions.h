/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DUNGEONRUNACTIONS_H
#define PLAYERBOTS_DUNGEONRUNACTIONS_H

#include <string>
#include <unordered_map>

#include "AttackAction.h"

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
    static constexpr float GROUP_RANGE = 35.0f;         // everyone this close before moving on
    static constexpr float READY_HEALTH_PCT = 60.0f;
    static constexpr float READY_MANA_PCT = 40.0f;
    static constexpr float STEP = 30.0f;                // how far ahead along the route the next waypoint is
    static constexpr float NODE_REACHED = 12.0f;        // counts as standing on a node
    static constexpr uint32 MAX_PULL_ATTEMPTS = 20;     // pulls issued on one pack before it is skipped
    static constexpr uint32 WAIT_LOG_INTERVAL_MS = 10000;
    static constexpr uint32 APPROACH_TIMEOUT_MS = 45000;

    void UpdateProgress(DungeonRoute const& route);
    bool ApproachTimedOut(uint32 index, float distance);
    bool TraceDue();  // throttles the debug trace to one line per WAIT_LOG_INTERVAL_MS
    bool GroupReady(std::string& reason) const;
    // First item on the path (not a side pack, not skipped) with a living, attackable member; sets the member
    // nearest to the bot.
    DungeonRouteItem const* NextItem(DungeonRoute const& route, Unit*& target, uint32& index) const;

    std::unordered_map<uint32, uint32> _pullAttempts;  // route item index -> pulls issued
    uint32 _lastWaitLogMs = 0;
    uint32 _approachItem = UINT32_MAX;  // item being walked to, best distance reached, since when
    float _approachBest = 0.0f;
    uint32 _approachSinceMs = 0;
    uint32 _lastApproachTickMs = 0;
    uint32 _lastTraceMs = 0;
};

#endif
