/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UKMultipliers.h"
#include "ChooseTargetActions.h"
#include "Creature.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "UKActions.h"
#include "UKTriggers.h"

#include <algorithm>
#include <list>

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

    // 暗影斧落在随机成员脚下，光环 42750 每秒在 5 码内触发一次 42751。麻烦不在规避
    // 动作本身——它被执行时是有效的——而在 `PlayerbotAI::UpdateAI`：自身施法处于
    // `SPELL_STATE_PREPARING` 时它会在跑引擎之前直接 return，于是那几个 tick 里
    // 触发器和动作一个都不会被求值。run250 的法师因此在斧上原地站了 4 秒、承受
    // 4 跳共 13,675。这里不打断已在读条的法术（那需要改 UpdateAI），而是在斧已经
    // 进入挨伤害的那一圈时不再起手**新的**非瞬发法术，让下一个 tick 能落到规避动作上。
    // 瞬发法术不受影响，半径也比规避动作的 12 码执行半径更紧，尽量少影响输出与治疗。
    if (dynamic_cast<CastSpellAction*>(action))
    {
        std::list<Creature*> axes;
        bot->GetCreatureListWithEntryInGrid(axes, NPC_THROW, kIngvarShadowAxeCastBlockRadius);
        bool const axeInDangerBand = std::any_of(axes.begin(), axes.end(), [](Creature const* axe)
        {
            return axe && axe->IsAlive();
        });
        if (axeInDangerBand)
        {
            uint32 const spellId = AI_VALUE2(uint32, "spell id", action->getName());
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
                if (spellInfo->CalcCastTime(bot) != 0)
                    return 0.0f;
        }
    }

    // Ingvar has a dedicated rear-position action with a validated 7-yard
    // stand-off.  The generic side-step action calculates its radius from the
    // bot's current melee overlap; during a moving boss transition that can
    // submit a point inside Ingvar's model.  Do not let that second controller
    // compete only while this bot is actually attacking Ingvar.
    if (!isTank && AI_VALUE(Unit*, "current target") == boss && dynamic_cast<SetBehindTargetAction*>(action))
        return 0.0f;

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
