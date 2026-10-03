/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AKACTIONS_H
#define PLAYERBOTS_AKACTIONS_H

#include "AKTriggers.h"
#include "Action.h"
#include "AttackAction.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

class AttackNadoxGuardianAction : public AttackAction
{
public:
    AttackNadoxGuardianAction(PlayerbotAI* ai) : AttackAction(ai, "attack nadox guardian") {}
    bool Execute(Event event) override;
};

class AttackJedogaVolunteerAction : public AttackAction
{
public:
    AttackJedogaVolunteerAction(PlayerbotAI* ai) : AttackAction(ai, "attack jedoga volunteer") {}
    bool Execute(Event event) override;
};

class AttackTaldaramEmbracingAction : public AttackAction
{
public:
    AttackTaldaramEmbracingAction(PlayerbotAI* ai) : AttackAction(ai, "attack taldaram embracing") {}
    bool Execute(Event event) override;
};

class AttackJedogaWorshipperAction : public AttackAction
{
public:
    AttackJedogaWorshipperAction(PlayerbotAI* ai) : AttackAction(ai, "attack jedoga worshipper") {}
    bool Execute(Event event) override;
};

class AvoidFlameSphereAction : public MovementAction
{
public:
    AvoidFlameSphereAction(PlayerbotAI* ai) : MovementAction(ai, "avoid flame sphere") {}
    bool Execute(Event event) override;
};

class AvoidShadowCrashAction : public MovementAction
{
public:
    AvoidShadowCrashAction(PlayerbotAI* ai) : MovementAction(ai, "avoid shadow crash") {}
    bool Execute(Event event) override;
};

#endif
