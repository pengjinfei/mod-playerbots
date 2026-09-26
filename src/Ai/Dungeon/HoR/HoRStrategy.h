/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORSTRATEGY_H
#define PLAYERBOTS_HORSTRATEGY_H

#include "Strategy.h"

// Halls of Reflection. Map 668 needs an instance strategy like every other WotLK dungeon; it also
// carries the Falric/Marwyn wave handling.
class WotlkDungeonHoRStrategy : public Strategy
{
public:
    WotlkDungeonHoRStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    std::string const getName() override { return "wotlk-hor"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
