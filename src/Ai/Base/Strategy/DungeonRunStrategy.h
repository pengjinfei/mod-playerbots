/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DUNGEONRUNSTRATEGY_H
#define PLAYERBOTS_DUNGEONRUNSTRATEGY_H

#include "NonCombatStrategy.h"

class PlayerbotAI;

// Opt-in, for the group leader: walk the map's dungeon route (AiPlayerbot.DungeonRouteDir) and pull what is on it.
// The rest of the group follows the leader; combat is handled by the usual combat strategies.
class DungeonRunStrategy : public NonCombatStrategy
{
public:
    DungeonRunStrategy(PlayerbotAI* botAI) : NonCombatStrategy(botAI) {}

    std::string const getName() override { return "dungeon run"; }
    // Replaces NonCombatStrategy::InitTriggers on purpose: its quest-log and mount triggers already come with "nc".
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif
