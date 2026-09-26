/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORSTRATEGY_H
#define PLAYERBOTS_HORSTRATEGY_H

#include "Strategy.h"

// Halls of Reflection. No encounter-specific behaviour yet: the strategy exists so map 668 has an
// instance strategy like every other WotLK dungeon, and gives Falric/Marwyn/Frostsworn General/the
// Lich King escape a place for their triggers.
class WotlkDungeonHoRStrategy : public Strategy
{
public:
    WotlkDungeonHoRStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    std::string const getName() override { return "wotlk-hor"; }
    void InitTriggers(std::vector<TriggerNode*>& /*triggers*/) override {}
};

#endif
