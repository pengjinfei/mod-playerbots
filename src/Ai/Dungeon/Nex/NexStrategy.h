/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NEXSTRATEGY_H
#define PLAYERBOTS_NEXSTRATEGY_H

#include "MarkRtiStrategy.h"
#include "Multiplier.h"
#include "Strategy.h"

// 继承 TrashCcPullStrategy：清怪控制链（指派/上控/不放 AoE/按序击杀）是共享能力，这里挂上并登记本副本的治疗小怪。
class WotlkDungeonNexStrategy : public TrashCcPullStrategy
{
public:
    WotlkDungeonNexStrategy(PlayerbotAI* ai);
    std::string const getName() override { return "wotlk-nex"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
