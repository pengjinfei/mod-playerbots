/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UKMultipliers.h"
#include "ChooseTargetActions.h"
#include "GenericSpellActions.h"
#include "UKActions.h"
#include "UKTriggers.h"

float PrinceKelesethMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "prince keleseth");
    if (!boss) { return 1.0f; }

    // Suppress auto-targeting behaviour only when a tomb is up
    if (dynamic_cast<DpsAssistAction*>(action))
    {
        GuidVector members = AI_VALUE(GuidVector, "group members");
        for (auto& member : members)
        {
            Unit* unit = botAI->GetUnit(member);
            if (unit && unit->HasAura(SPELL_FROST_TOMB))
            {
                return 0.0f;
            }
        }
    }
    return 1.0f;
}

float SkarvaldAndDalronnMultiplier::GetValue(Action* action)
{
    // Only need to deal with Dalronn here. If he's dead, just fall back to normal dps strat
    Unit* dalronn = AI_VALUE2(Unit*, "find target", "dalronn the controller");
    if (!dalronn) { return 1.0f; }

    // Only suppress DpsAssistAction if Dalronn is alive
    if (dalronn->isTargetableForAttack() && dynamic_cast<DpsAssistAction*>(action))
    {
        return 0.0f;
    }
    return 1.0f;
}

float IngvarThePlundererMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    bool isTank = botAI->IsTank(bot);
    if (!boss) { return 1.0f; }

    // Prevent arbitrary movement from overriding a tank dodge, but keep both
    // documented Ingvar responses available.  Shadow Axe can select the tank;
    // excluding that response leaves the tank in repeated axe hits until the
    // whole party loses its only stable target.
    if (isTank && bot->isMoving() && dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<IngvarDodgeSmashAction*>(action) &&
        !dynamic_cast<IngvarAvoidShadowAxeAction*>(action))
    {
        return 0.0f;
    }

    // If boss is casting a roar, do not allow beginning a spell cast that is non-instant
    if (boss->HasUnitState(UNIT_STATE_CASTING))
    {
        if (boss->FindCurrentSpellBySpellId(SPELL_STAGGERING_ROAR) ||
            boss->FindCurrentSpellBySpellId(SPELL_DREADFUL_ROAR))
        {
            if (dynamic_cast<CastSpellAction*>(action))
            {
                uint32 spellId = AI_VALUE2(uint32, "spell id", action->getName());
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
                if (!spellInfo) { return 1.0f; }

                uint32 castTime = spellInfo->CalcCastTime(bot);
                if (castTime != 0)
                {
                    return 0.0f;
                }
            }
        }
        // Done with non-tank logic
        if (!isTank) { return 1.0f; }

        // TANK ONLY
        if (boss->FindCurrentSpellBySpellId(SPELL_SMASH) ||
            boss->FindCurrentSpellBySpellId(SPELL_DARK_SMASH))
        {
            // Prevent movement actions during smash which can mess up boss position.
            // Shadow Axe remains an immediate hazard, including when it is aimed
            // at the tank, so allow both dedicated evasion actions through.
            if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<IngvarDodgeSmashAction*>(action) &&
                !dynamic_cast<IngvarAvoidShadowAxeAction*>(action))
            {
                return 0.0f;
            }
        }
    }
    return 1.0f;
}
