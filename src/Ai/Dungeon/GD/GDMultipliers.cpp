/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GDMultipliers.h"
#include "Action.h"
#include "AttackAction.h"
#include "ChooseTargetActions.h"
#include "GDActions.h"
#include "GDTriggers.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "Playerbots.h"
#include "ReachTargetActions.h"

float SladranMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "slad'ran");
    if (!boss) { return 1.0f; }

    if (boss->FindCurrentSpellBySpellId(SPELL_POISON_NOVA))
    {
        if (dynamic_cast<AvoidPoisonNovaAction*>(action)) { return 1.0f; }

        if (dynamic_cast<MovementAction*>(action))
        {
            return 0.0f;
        }
    }

    if (dynamic_cast<FleeAction*>(action) ||
        dynamic_cast<RunAwayAction*>(action) ||
        dynamic_cast<MoveRandomAction*>(action) ||
        dynamic_cast<MoveFromGroupAction*>(action))
    {
        return 0.0f;
    }

    if (PlayerbotAI::IsTank(bot))
    {
        if (dynamic_cast<TankAssistAction*>(action))
        {
            Unit* tankTarget = AI_VALUE(Unit*, "tank target");
            if (tankTarget && GundrakSladran::IsAdd(tankTarget) &&
                bot->GetExactDist2d(tankTarget) > GundrakSladran::TANK_PICKUP_YD)
            {
                return 0.0f;
            }
        }

        if (dynamic_cast<ReachTargetAction*>(action))
        {
            Unit* currentTarget = AI_VALUE(Unit*, "current target");
            if (currentTarget && GundrakSladran::IsAdd(currentTarget) &&
                bot->GetExactDist2d(currentTarget) > GundrakSladran::TANK_PICKUP_YD)
            {
                return 0.0f;
            }
        }

        return 1.0f;
    }

    if (dynamic_cast<CombatFormationMoveAction*>(action) &&
        !dynamic_cast<SetBehindTargetAction*>(action))
    {
        return 0.0f;
    }

    if (dynamic_cast<DpsAssistAction*>(action) && GundrakSladran::GetAssignedSnakeWrap(botAI))
    {
        return 0.0f;
    }

    return 1.0f;
}

float GaldarahMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gal'darah");
    if (!boss) { return 1.0f; }

    if (boss->HasAura(SPELL_WHIRLING_SLASH))
        {
            if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<AvoidWhirlingSlashAction*>(action))
            {
                return 0.0f;
            }
        }
    return 1.0f;
}

float MoorabiLancerRetaliationMultiplier::GetValue(Action* action)
{
    // run681–683: Lancer 保持 tank victim 时，盗贼仍在 40546 窗口攻击并被 22858
    // 连续反伤击杀。只拦截「当前目标」上的攻击；self 防御、移动、治疗和其它职业不受影响。
    if (bot->getClass() != CLASS_ROGUE)
        return 1.0f;

    Unit* target = bot->GetVictim();
    if (!target)
        target = AI_VALUE(Unit*, "current target");
    if (!target || target->GetEntry() != NPC_DRAKKARI_LANCER || !target->HasAura(SPELL_LANCER_RETALIATION))
        return 1.0f;

    if (dynamic_cast<AttackAction*>(action))
        return 0.0f;

    if (CastSpellAction* spell = dynamic_cast<CastSpellAction*>(action))
        return spell->GetTargetName() == "current target" ? 0.0f : 1.0f;

    return 1.0f;
}
