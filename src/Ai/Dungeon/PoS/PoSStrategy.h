/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_POSSTRATEGY_H
#define PLAYERBOTS_POSSTRATEGY_H

#include "MarkRtiStrategy.h"

// Inherits TrashCcPullStrategy, the shared trash crowd-control chain, as the Forge of Souls does: without it the leader
// of a full run waits for marks no one sets and pulls whole packs uncontrolled.
class WotlkDungeonPoSStrategy : public TrashCcPullStrategy
{
public:
    WotlkDungeonPoSStrategy(PlayerbotAI* ai) : TrashCcPullStrategy(ai) {}
    std::string const getName() override { return "wotlk-pos"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
