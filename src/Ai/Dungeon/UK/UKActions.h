/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_UKACTIONS_H
#define PLAYERBOTS_UKACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "UKTriggers.h"

class AttackFrostTombAction : public AttackAction
{
public:
    AttackFrostTombAction(PlayerbotAI* ai) : AttackAction(ai, "attack frost tomb") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class AttackDalronnAction : public AttackAction
{
public:
    AttackDalronnAction(PlayerbotAI* ai) : AttackAction(ai, "attack dalronn") {}
    bool Execute(Event event) override;
};

class IngvarDodgeSmashAction : public MovementAction
{
public:
    IngvarDodgeSmashAction(PlayerbotAI* ai) : MovementAction(ai, "ingvar dodge smash") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class IngvarGetBehindAction : public MovementAction
{
public:
    IngvarGetBehindAction(PlayerbotAI* ai, std::string const& name = "ingvar get behind") : MovementAction(ai, name) {}
    bool Execute(Event event) override;

protected:
    bool MoveBehind(Unit* boss, MovementPriority priority, char const* reason);
};

class IngvarEvadeDarkSmashAction : public IngvarGetBehindAction
{
public:
    IngvarEvadeDarkSmashAction(PlayerbotAI* ai) : IngvarGetBehindAction(ai, "ingvar evade dark smash") {}
    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;
};

class IngvarClearContactAction : public IngvarGetBehindAction
{
public:
    IngvarClearContactAction(PlayerbotAI* ai) : IngvarGetBehindAction(ai, "ingvar clear contact") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class IngvarAvoidShadowAxeAction : public MovementAction
{
public:
    IngvarAvoidShadowAxeAction(PlayerbotAI* ai) : MovementAction(ai, "ingvar avoid shadow axe") {}
    bool Execute(Event event) override;
    bool isUseful() override;

private:
    // Diagnostic-only: log the 20 yd trigger / 12 yd execution threshold once per axe.
    ObjectGuid _lastLoggedAxe;
};

#endif
