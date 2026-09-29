/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_UKSTRATEGY_H
#define PLAYERBOTS_UKSTRATEGY_H

#include "MarkRtiStrategy.h"

// Inherits TrashCcPullStrategy: the trash crowd-control chain (assign / cast before the pull / no AoE / kill in order)
// is shared; a leader pulling the next pack (dungeon run) gets it by pinning "pull target" before the pull.
class WotlkDungeonUKStrategy : public TrashCcPullStrategy
{
public:
    WotlkDungeonUKStrategy(PlayerbotAI* ai);
    std::string const getName() override { return "wotlk-uk"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
