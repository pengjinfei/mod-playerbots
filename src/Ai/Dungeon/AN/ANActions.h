/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ANACTIONS_H
#define PLAYERBOTS_ANACTIONS_H

#include "ANTriggers.h"
#include "Action.h"
#include "AttackAction.h"
#include "MovementActions.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

class AttackWebWrapAction : public AttackAction
{
public:
    AttackWebWrapAction(PlayerbotAI* ai) : AttackAction(ai, "attack web wrap") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class WatchersTargetAction : public AttackAction
{
public:
    WatchersTargetAction(PlayerbotAI* ai) : AttackAction(ai, "krik'thir priority") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class AnubarakDodgePoundAction : public AttackAction
{
public:
    AnubarakDodgePoundAction(PlayerbotAI* ai) : AttackAction(ai, "anub'arak dodge pound") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// 躲开穿刺尖刺：尖刺生成后 4 秒才落伤害、半径只有 4 码，走开两步就行。
class AnubarakDodgeImpaleAction : public MovementAction
{
public:
    AnubarakDodgeImpaleAction(PlayerbotAI* ai) : MovementAction(ai, "anub'arak dodge impale") {}
    bool Execute(Event event) override;
};

#endif
