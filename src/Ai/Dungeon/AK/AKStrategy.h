/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AKSTRATEGY_H
#define PLAYERBOTS_AKSTRATEGY_H

#include "MarkRtiStrategy.h"

// Inherits TrashCcPullStrategy, the shared trash crowd-control chain, like the other dungeon strategies (DTK, PoS, UK,
// FoS, AN, GD, Nex). The chain was factored out of Ai/Dungeon/Nex on 2026-09-12, after this strategy was written, and
// Ahn'kahet was never brought over: without it the leader of a full run "waits for marks no one sets and pulls whole
// packs uncontrolled" (PoSStrategy.h). Measured here on 2026-10-11: the spike windows that kill runs carry 5.0 enemies
// on average against 2.8 in the quiet ones, and 7.6x the damage taken.
class WotlkDungeonOKStrategy : public TrashCcPullStrategy
{
public:
    WotlkDungeonOKStrategy(PlayerbotAI* ai) : TrashCcPullStrategy(ai) {}
    std::string const getName() override { return "wotlk-ok"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
