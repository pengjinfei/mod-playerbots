/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DTKACTIONS_H
#define PLAYERBOTS_DTKACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "DTKTriggers.h"
#include "GenericSpellActions.h"
#include "PlayerbotAI.h"
#include "MovementActions.h"
#include "Playerbots.h"

const Position NOVOS_PARTY_POSITION = Position(-378.852f, -760.349f, 28.587f);

class CorpseExplodeSpreadAction : public MovementAction
{
public:
    CorpseExplodeSpreadAction(PlayerbotAI* ai) : MovementAction(ai, "corpse explode spread") {}
    bool Execute(Event event) override;
};

class AvoidArcaneFieldAction : public MovementAction
{
public:
    AvoidArcaneFieldAction(PlayerbotAI* ai) : MovementAction(ai, "avoid arcane field") {}
    bool Execute(Event event) override;
};

class NovosDefaultPositionAction : public MovementAction
{
public:
    NovosDefaultPositionAction(PlayerbotAI* ai) : MovementAction(ai, "novos default position") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class NovosTargetPriorityAction : public AttackAction
{
public:
    NovosTargetPriorityAction(PlayerbotAI* ai) : AttackAction(ai, "novos target priority") {}
    bool Execute(Event event) override;
    // bool isUseful() override;
};

class CastSlayingStrikeAction : public CastMeleeSpellAction
{
public:
    CastSlayingStrikeAction(PlayerbotAI* botAI) : CastMeleeSpellAction(botAI, "slaying strike") {}
};

class CastTauntAction : public CastSpellAction
{
public:
    CastTauntAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "taunt") {}
};

class CastBoneArmorAction : public CastSpellAction
{
public:
    CastBoneArmorAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "bone armor") {}
};

class CastTouchOfLifeAction : public CastSpellAction
{
public:
    CastTouchOfLifeAction(PlayerbotAI* botAI) : CastSpellAction(botAI, "touch of life") {}
};

// Heal the member with Grievous Bite to full: the bleed ends only at full health. In range and in sight first - the
// healer stood at the stairs top, out of sight of the tank it should have healed (my-mac run100083).
class GrievousBiteReachAction : public MovementAction
{
public:
    GrievousBiteReachAction(PlayerbotAI* ai) : MovementAction(ai, "grievous bite reach") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class GrievousBiteHealAction : public CastSpellAction
{
public:
    GrievousBiteHealAction(PlayerbotAI* ai);
    std::string const getName() override { return "grievous bite heal"; }
    Unit* GetTarget() override { return FindGrievousBiteTarget(bot); }
    bool isUseful() override;
};

#endif
