/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_MARKRTISTRATEGY_H
#define PLAYERBOTS_MARKRTISTRATEGY_H

#include "Strategy.h"

class PlayerbotAI;

class MarkRtiStrategy : public Strategy
{
public:
    MarkRtiStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "mark rti"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

// 清怪控制链的策略基座（docs/testing/TRASH-CC-PULL-DESIGN.md）：坦克按队伍图标指派（骷髅=先杀、月亮=法师羊、
// 方块=萨满妖术、十字=盗贼闷棍）→ 闷棍先、羊/妖术后 → 编排层等控制落地再开怪 → 有控制不放 AoE（全局乘子）→
// 单体按序、被控的最后杀。副本策略继承它并调用 InitTriggers，需要的话用 TrashCcRegisterHealerEntries 登记治疗小怪。
// 2026-09-12 从 Ai/Dungeon/Nex 归位；实测见台账 run407–414。
class TrashCcPullStrategy : public Strategy
{
public:
    TrashCcPullStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    bool HasTargetExclusions() const override { return true; }
    void AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType type) override;
};

#endif
