/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_CHOOSETARGETACTIONS_H
#define PLAYERBOTS_CHOOSETARGETACTIONS_H

#include "AttackAction.h"

class PlayerbotAI;

class DpsAoeAction : public AttackAction
{
public:
    DpsAoeAction(PlayerbotAI* botAI) : AttackAction(botAI, "dps aoe") {}

    std::string const GetTargetName() override { return "dps aoe target"; }
};

class DpsAssistAction : public AttackAction
{
public:
    DpsAssistAction(PlayerbotAI* botAI) : AttackAction(botAI, "dps assist") {}

    std::string const GetTargetName() override { return "dps target"; }
    bool isUseful() override;
};

class TankAssistAction : public AttackAction
{
public:
    TankAssistAction(PlayerbotAI* botAI) : AttackAction(botAI, "tank assist") {}

    std::string const GetTargetName() override { return "tank target"; }
    bool Execute(Event event) override;
};

class AggressiveTargetAction : public AttackAction
{
public:
    AggressiveTargetAction(PlayerbotAI* botAI) : AttackAction(botAI, "aggressive target") {}

    std::string const GetTargetName() override { return "aggressive target"; }
    bool isUseful() override;
};

class AttackAnythingAction : public AttackAction
{
public:
    AttackAnythingAction(PlayerbotAI* botAI) : AttackAction(botAI, "attack anything") {}

    std::string const GetTargetName() override { return "grind target"; }
    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;
};

class AttackLeastHpTargetAction : public AttackAction
{
public:
    AttackLeastHpTargetAction(PlayerbotAI* botAI) : AttackAction(botAI, "attack least hp target") {}

    std::string const GetTargetName() override { return "least hp target"; }
};

class AttackEnemyPlayerAction : public AttackAction
{
public:
    AttackEnemyPlayerAction(PlayerbotAI* botAI) : AttackAction(botAI, "attack enemy player") {}

    std::string const GetTargetName() override { return "enemy player target"; }
    bool isUseful() override;
};

class AttackRtiTargetAction : public AttackAction
{
public:
    AttackRtiTargetAction(PlayerbotAI* botAI) : AttackAction(botAI, "attack rti target") {}

    std::string const GetTargetName() override { return "rti target"; }
    bool Execute(Event event) override;
    bool isUseful() override;
};

class AttackEnemyFlagCarrierAction : public AttackAction
{
public:
    AttackEnemyFlagCarrierAction(PlayerbotAI* botAI) : AttackAction(botAI, "attack enemy flag carrier") {}

    std::string const GetTargetName() override { return "enemy flag carrier"; }
    bool isUseful() override;
};

// AttackAction 而不是 Action：目标死掉之后要在同一个 tick 里直接接管下一个目标，需要 Attack()。
class DropTargetAction : public AttackAction
{
public:
    DropTargetAction(PlayerbotAI* botAI) : AttackAction(botAI, "drop target") {}

    bool Execute(Event event) override;
    // "invalid target" also fires when there is no target at all (InvalidTargetValue returns !target). With the bot now
    // kept in the combat engine while in combat, an empty target would make drop target (relevance 99) win every tick
    // and starve everything else (run 491/1: healer 103 drops, 0 heals, wipe at 45 s). Nothing to drop -> useless,
    // unless we are out of combat and should hand over to the non-combat engine.
    bool isUseful() override;
};

#endif
