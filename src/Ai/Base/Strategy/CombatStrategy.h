/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_COMBATSTRATEGY_H
#define PLAYERBOTS_COMBATSTRATEGY_H

#include "Strategy.h"

class PlayerbotAI;

class CombatStrategy : public Strategy
{
public:
    CombatStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    // 所有职业的战斗策略都派生自这里（MeleeCombatStrategy / RangedCombatStrategy），
    // 所以这一个接入点就能给全职业挂上「别用自家 AoE 打破自家控制」。
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
    uint32 GetType() const override { return STRATEGY_TYPE_COMBAT; }
};

class AvoidAoeStrategy : public Strategy
{
public:
    explicit AvoidAoeStrategy(PlayerbotAI* ai);
    const std::string getName() override { return "avoid aoe"; }
    std::vector<NextAction> getDefaultActions() override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

class TankFaceStrategy : public Strategy
{
public:
    explicit TankFaceStrategy(PlayerbotAI* ai);
    const std::string getName() override { return "tank face"; }
    std::vector<NextAction> getDefaultActions() override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

class CombatFormationStrategy : public Strategy
{
public:
    CombatFormationStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    const std::string getName() override { return "formation"; }
    std::vector<NextAction> getDefaultActions() override;
};

#endif
