/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DTKSTRATEGY_H
#define PLAYERBOTS_DTKSTRATEGY_H

#include "MarkRtiStrategy.h"

// Inherits TrashCcPullStrategy like the other dungeon strategies: without it a dungeon-run leader's cc gate never
// got a plan here ("no_plan") and pulled the first hall's undead uncontrolled (my-mac run100057).
class WotlkDungeonDTKStrategy : public TrashCcPullStrategy
{
public:
    WotlkDungeonDTKStrategy(PlayerbotAI* ai) : TrashCcPullStrategy(ai) {}
    std::string const getName() override { return "wotlk-dtk"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
