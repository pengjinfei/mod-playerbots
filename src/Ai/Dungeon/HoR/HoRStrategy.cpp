/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRStrategy.h"
#include "HoRMultipliers.h"

void WotlkDungeonHoRStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("hor wave boss boost",
        { NextAction("heroism", ACTION_RAID + 1), NextAction("bloodlust", ACTION_RAID + 1) }));
    triggers.push_back(new TriggerNode("hor escape keep up",
        { NextAction("hor escape keep up", ACTION_RAID + 4) }));
}

void WotlkDungeonHoRStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new HoRHoldHeroismForBossMultiplier(botAI));
}
