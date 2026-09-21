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
};

// 把 DPS 的目标拉回斯拉德兰本人。
class SladranFocusBossAction : public AttackAction
{
public:
    SladranFocusBossAction(PlayerbotAI* ai) : AttackAction(ai, "slad'ran focus boss") {}
    bool Execute(Event event) override;
};

class SladranStackOnTankAction : public MovementAction
{
public:
    SladranStackOnTankAction(PlayerbotAI* ai) : MovementAction(ai, "slad'ran stack on tank") {}
    bool Execute(Event event) override;
};

class SladranTankHoldAction : public AttackAction
{
public:
    SladranTankHoldAction(PlayerbotAI* ai) : AttackAction(ai, "slad'ran tank hold") {}
    bool Execute(Event event) override;
};

class AvoidWhirlingSlashAction : public MovementAction
{
public:
    AvoidWhirlingSlashAction(PlayerbotAI* ai) : MovementAction(ai, "avoid whirling slash") {}
    bool Execute(Event event) override;
};

// 只在 29819 的 40546 窗口停止盗贼的白字攻击；技能动作由同名 multiplier
// 精确过滤，窗口外立即恢复。不是 raidtest 编排动作，也不改变仇恨。
class MoorabiLancerRetaliationWaitAction : public Action
{
public:
    MoorabiLancerRetaliationWaitAction(PlayerbotAI* ai) : Action(ai, "moorabi lancer retaliation wait") {}
    bool Execute(Event event) override
    {
        (void)event;
        bot->AttackStop();
        return true;
    }
};

#endif
