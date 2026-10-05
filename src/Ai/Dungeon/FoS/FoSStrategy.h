/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_FOSSTRATEGY_H
#define PLAYERBOTS_FOSSTRATEGY_H

#include "MarkRtiStrategy.h"

// Inherits TrashCcPullStrategy, the shared trash crowd-control chain: the Soulguards come four at a time, reapers and
// adepts humanoid, bonecasters undead, and fought uncontrolled the four on the ramp before Bronjahm took the group
// down three runs in a row with the damage spread over all of them (full run, runs 2218-2220).
class WotlkDungeonFoSStrategy : public TrashCcPullStrategy
{
public:
    WotlkDungeonFoSStrategy(PlayerbotAI* ai) : TrashCcPullStrategy(ai) {}
    std::string const getName() override { return "wotlk-fos"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
