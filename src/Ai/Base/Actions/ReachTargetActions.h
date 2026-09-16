/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_REACHTARGETACTIONS_H
#define PLAYERBOTS_REACHTARGETACTIONS_H

#include "GenericSpellActions.h"
#include "MovementActions.h"

class PlayerbotAI;

class ReachTargetAction : public MovementAction
{
public:
    ReachTargetAction(PlayerbotAI* botAI, std::string const name, float distance)
        : MovementAction(botAI, name), distance(distance)
    {
    }

    bool Execute(Event event) override;
    bool isUseful() override;
    std::string const GetTargetName() override;
    // Closing distance is positioning: if another target can be cast on right now, stop and cast.
    MovementIntent GetMovementIntent() const override { return MovementIntent::POSITIONING; }
    // True when the target is an enemy the bot wants to attack (reach spell / reach melee). Enemy chasing is
    // leashed to the main tank's fight for non-tanks; reaching a party member to heal or resurrect is not.
    virtual bool ChasesEnemy() const { return true; }

protected:
    // Is the target outside the group's fight: heal range around the main tank (non-tanks) or around the healer (tanks)?
    bool IsChaseLeashed(Unit* target) const;
    Unit* ChaseAnchor() const;

    float distance;
};

class CastReachTargetSpellAction : public CastSpellAction
{
public:
    CastReachTargetSpellAction(PlayerbotAI* botAI, std::string const spell, float distance)
        : CastSpellAction(botAI, spell), distance(distance)
    {
    }

    bool isUseful() override;

protected:
    float distance;
};

class ReachMeleeAction : public ReachTargetAction
{
public:
    ReachMeleeAction(PlayerbotAI* botAI) : ReachTargetAction(botAI, "reach melee", sPlayerbotAIConfig.meleeDistance) {}
};

class ReachSpellAction : public ReachTargetAction
{
public:
    ReachSpellAction(PlayerbotAI* botAI);
};

// 「走到治疗目标看得见的地方」。两处与基类不同，都是为了修同一个缺陷
// （看不见的队友被当成不存在，治疗静默停摆）：
//   1. 取值用 "party member to heal no los"，否则视线一断目标就是空，连走都不会走；
//   2. isUseful/Execute 把「距离够但看不见」也算需要移动——基类的 IsWithinCombatRange
//      一为真就返回 false，而 ReachCombatTo 在距离已够时同样直接返回 false。
class ReachPartyMemberToHealAction : public ReachTargetAction
{
public:
    ReachPartyMemberToHealAction(PlayerbotAI* botAI);

    std::string const GetTargetName() override;
    bool ChasesEnemy() const override { return false; }
    bool isUseful() override;
    bool Execute(Event event) override;
};

class ReachPartyMemberToResurrectAction : public ReachTargetAction
{
public:
    ReachPartyMemberToResurrectAction(PlayerbotAI* botAI);

    std::string const GetTargetName() override;
    bool ChasesEnemy() const override { return false; }
};

#endif
