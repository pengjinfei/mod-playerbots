/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_TOCACTIONS_H
#define PLAYERBOTS_TOCACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "EquipAction.h"
#include "Event.h"
#include "ItemCountValue.h"
#include "ItemUsageValue.h"
#include "LastMovementValue.h"
#include "MovementActions.h"
#include "ObjectGuid.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "ScriptedCreature.h"
#include "SharedDefines.h"
#include "StatsWeightCalculator.h"
#include "TOCTriggers.h"

class ToCLanceAction : public AttackAction
{
public:
    ToCLanceAction(PlayerbotAI* ai) : AttackAction(ai, "toc lance") {}
    bool Execute(Event event) override;
};

class ToCUELanceAction : public AttackAction
{
public:
    ToCUELanceAction(PlayerbotAI* ai) : AttackAction(ai, "toc ue lance") {}
    bool Execute(Event event) override;
};

class ToCMountedAction : public Action
{
public:
    ToCMountedAction(PlayerbotAI* botAI, std::string const name = "toc mounted")
        : Action(botAI, name) {}
    bool Execute(Event event) override;
};

class ToCMountAction : public MovementAction
{
public:
    ToCMountAction(PlayerbotAI* botAI, std::string const name = "toc mount")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
    bool EnterVehicle(Unit* vehicleBase, bool moveIfFar);
};

class ToCEadricAction : public MovementAction
{
public:
    ToCEadricAction(PlayerbotAI* botAI, std::string const name = "toc eadric")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

// Reflective Shield sends a share of every hit back to the attacker (heroic run1211: 884 reflected hits, 152k
// damage) while her Memory, the add she summons at the same moment, is left alone. Move off her onto the Memory
// while it lives; the shield has to be broken on her afterwards.
class ToCPaletressShieldAction : public AttackAction
{
public:
    ToCPaletressShieldAction(PlayerbotAI* botAI) : AttackAction(botAI, "toc paletress shield") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};
#endif
