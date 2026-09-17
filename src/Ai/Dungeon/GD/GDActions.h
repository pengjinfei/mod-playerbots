/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_GDACTIONS_H
#define PLAYERBOTS_GDACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "GDTriggers.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

class AvoidPoisonNovaAction : public MovementAction
{
public:
    AvoidPoisonNovaAction(PlayerbotAI* ai) : MovementAction(ai, "avoid poison nova") {}
    bool Execute(Event event) override;
};

class AttackSnakeWrapAction : public AttackAction
{
public:
    AttackSnakeWrapAction(PlayerbotAI* ai) : AttackAction(ai, "attack snake wrap") {}
    bool Execute(Event event) override;

private:
    // 找出这个包裹困住的是谁（召唤者优先，读不到就取 8 码内最近的队友）
    Player* ResolveWrapVictim(Unit* wrap, char const** how = nullptr);
};

// 把 DPS 的目标拉回斯拉德兰本人。
class SladranFocusBossAction : public AttackAction
{
public:
    SladranFocusBossAction(PlayerbotAI* ai) : AttackAction(ai, "slad'ran focus boss") {}
    bool Execute(Event event) override;
};

class AvoidWhirlingSlashAction : public MovementAction
{
public:
    AvoidWhirlingSlashAction(PlayerbotAI* ai) : MovementAction(ai, "avoid whirling slash") {}
    bool Execute(Event event) override;
};

#endif
