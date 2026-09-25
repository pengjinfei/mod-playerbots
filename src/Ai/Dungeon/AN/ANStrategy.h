/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ANSTRATEGY_H
#define PLAYERBOTS_ANSTRATEGY_H

#include "MarkRtiStrategy.h"
#include "Multiplier.h"
#include "Strategy.h"

// 继承清怪控制链（docs/testing/TRASH-CC-PULL-DESIGN.md）。本副本整本小怪都是亡灵
// （creature_template.type = 6），变形术/妖术/闷棍的 TargetCreatureType 都不含亡灵，
// 唯一能落地的控制是牧师的束缚亡灵——共享层的分工表已经涵盖，这里不需要副本特有代码。
// 门厅那 9 只没有治疗小怪（技能只有致盲蛛网/缠网/毒液/暗影箭之流），所以不登记
// TrashCcRegisterHealerEntries，按共享层默认的「有法力 > 其它」排，暗影术士(28733) 会被优先控。
class WotlkDungeonANStrategy : public TrashCcPullStrategy
{
public:
    WotlkDungeonANStrategy(PlayerbotAI* ai) : TrashCcPullStrategy(ai) {}
    std::string const getName() override { return "wotlk-an"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
