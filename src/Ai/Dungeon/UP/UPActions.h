/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_UPACTIONS_H
#define PLAYERBOTS_UPACTIONS_H

#include "MovementActions.h"

class AvoidFreezingCloudAction : public MovementAction
{
public:
    AvoidFreezingCloudAction(PlayerbotAI* ai) : MovementAction(ai, "avoid freezing cloud") {}
    bool Execute(Event event) override;
};

class AvoidSkadiWhirlwindAction : public MovementAction
{
public:
    AvoidSkadiWhirlwindAction(PlayerbotAI* ai) : MovementAction(ai, "avoid skadi whirlwind") {}
    bool Execute(Event event) override;
};

// Bane reflects every hit on King Ymiron as shadow damage to the whole group. The generic "drop target" only clears
// the target value, leaves melee auto-attack swinging and is itself an AttackAction that the Bane multiplier blocks.
class YmironBaneStopAttackAction : public Action
{
public:
    YmironBaneStopAttackAction(PlayerbotAI* ai) : Action(ai, "ymiron bane stop attack") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

#endif
