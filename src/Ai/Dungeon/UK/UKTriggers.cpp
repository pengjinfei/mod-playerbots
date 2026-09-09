/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UKTriggers.h"
#include "AiObjectContext.h"
#include "Creature.h"
#include "Log.h"
#include "Playerbots.h"
#include "Spell.h"
#include "Unit.h"

#include <algorithm>
#include <list>

namespace
{
    constexpr float kIngvarDarkSmashConeRadians = 1.04719755f;
}

Unit* FindIngvarCrowdingMember(PlayerbotAI* botAI, Player* bot)
{
    Unit* nearest = nullptr;
    float nearestDistance = kIngvarSpreadRadius;
    for (ObjectGuid const& guid : botAI->GetAiObjectContext()->GetValue<GuidVector>("group members")->Get())
    {
        if (guid == bot->GetGUID())
            continue;

        Unit* member = botAI->GetUnit(guid);
        if (!member || !member->IsAlive() || member->GetMapId() != bot->GetMapId())
            continue;

        float const distance = bot->GetExactDist2d(member);
        if (distance < nearestDistance)
        {
            nearest = member;
            nearestDistance = distance;
        }
    }
    return nearest;
}

bool KelesethFrostTombTrigger::IsActive()
{
    GuidVector members = AI_VALUE(GuidVector, "group members");
    for (auto& member : members)
    {
        Unit* unit = botAI->GetUnit(member);
        if (unit && unit->HasAura(SPELL_FROST_TOMB))
        {
            return true;
        }
    }
    return false;
}

bool DalronnDpsTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "dalronn the controller");
    if (!boss || !boss->isTargetableForAttack()) { return false; }

    // This doesn't cause issues with healers currently and they will continue to heal even when included here
    return !botAI->IsTank(bot);
}

bool IngvarDreadfulRoarTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss) { return false; }

    if (boss->FindCurrentSpellBySpellId(SPELL_DREADFUL_ROAR))
    {
        return true;
    }
    return false;
}

bool IngvarSmashTankTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss || !botAI->IsTank(bot)) { return false; }

    // Ingvar's script casts the base spell IDs (42669/42723). The spell system
    // resolves their heroic effects to 59706/59709. Its instantaneous cast is
    // not reliably visible as a current Spell to playerbot ticks, but the script
    // roots the boss for exactly the 3.75 s smash window.
    if (boss->FindCurrentSpellBySpellId(SPELL_SMASH_N) ||
        boss->FindCurrentSpellBySpellId(SPELL_DARK_SMASH_N) || boss->HasUnitState(UNIT_STATE_ROOT))
    {
        LOG_DEBUG("playerbots", "Ingvar diagnostic: smash trigger active bot={} boss={} dist={:.2f}",
                  bot->GetName(), boss->GetGUID().ToString(), bot->GetExactDist2d(boss));
        return true;
    }
    return false;
}

bool IngvarDarkSmashNonTankTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss || botAI->IsTank(bot) || boss->GetDisplayId() != INGVAR_UNDEAD_DISPLAY_ID ||
        !boss->HasUnitState(UNIT_STATE_ROOT))
        return false;

    // The hero spell's unique-target list is the union of its effects. Only
    // effect 0 is the 60 degree front cone; move non-tanks out of that arc.
    // Do not require current target == boss: a healer or targetless member can
    // be in the cone even when it is not currently attacking Ingvar.
    // HasInArc carries no range term. Effect 0 stops at 10 yd (SpellRadius index 13),
    // so a member beyond that radius is not in the cone at all.
    bool const inFrontCone = boss->HasInArc(kIngvarDarkSmashConeRadians, bot) &&
        bot->GetExactDist2d(boss) <= kIngvarSmashConeRadius;
    if (inFrontCone)
    {
        Unit* currentTarget = AI_VALUE(Unit*, "current target");
        LOG_DEBUG("playerbots", "Ingvar diagnostic: dark-smash trigger bot={} boss={} current_target={} "
                                   "target_is_boss={} "
                                   "front_60=true",
                  bot->GetName(), boss->GetGUID().ToString(),
                  currentTarget ? currentTarget->GetGUID().ToString() : "none", currentTarget == boss);
    }
    return inFrontCone;
}

bool IngvarContactClearanceTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    bool const active = boss && !botAI->IsTank(bot) && bot->GetExactDist2d(boss) <= 1.5f;
    if (active)
        LOG_DEBUG("playerbots", "Ingvar diagnostic: contact-clearance trigger bot={} distance={:.2f}",
                  bot->GetName(), bot->GetExactDist2d(boss));
    return active;
}

bool NotBehindIngvarTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    if (!boss || currentTarget != boss || botAI->IsTank(bot) || botAI->IsHeal(bot)) { return false; }

    // The rear arc is melee's answer to the 10 yd front cone, because melee cannot
    // leave the cone's radius and keep contact. Members that do not need contact are
    // handled by `ingvar ranged clearance` instead and must not be pulled inwards here.
    if (botAI->IsRanged(bot))
        return false;

    return !AI_VALUE2(bool, "behind", "current target");
}

bool IngvarRangedClearanceTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss || botAI->IsTank(bot))
        return false;

    if (!botAI->IsRanged(bot) && !botAI->IsHeal(bot))
        return false;

    // Effect 0 of Smash / Dark Smash cannot select anything past 10 yd, and the boss
    // turns to its victim constantly, so for these roles the safe criterion is range
    // rather than the momentary rear arc.
    if (!bot->IsInCombat() || !boss->IsInCombat())
        return false;

    bool const active = bot->GetExactDist2d(boss) < kIngvarRangedClearance;
    if (active)
        LOG_DEBUG("playerbots", "Ingvar diagnostic: ranged-clearance trigger bot={} distance={:.2f}",
                  bot->GetName(), bot->GetExactDist2d(boss));
    return active;
}

bool IngvarSpreadTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss || botAI->IsTank(bot))
        return false;

    // Only hold a spread position once the encounter is running: before the pull the
    // party is still on its prepared marks and must not scatter off them.
    if (!bot->IsInCombat() || !boss->IsInCombat())
        return false;

    // Melee has to keep contact, so it cannot trade its position for separation.
    if (!botAI->IsRanged(bot) && !botAI->IsHeal(bot))
        return false;

    Unit* crowd = FindIngvarCrowdingMember(botAI, bot);
    if (crowd)
        LOG_DEBUG("playerbots", "Ingvar diagnostic: spread trigger bot={} crowd={} distance={:.2f}",
                  bot->GetName(), crowd->GetName(), bot->GetExactDist2d(crowd));
    return crowd != nullptr;
}

bool IngvarShadowAxeTrigger::IsActive()
{
    std::list<Creature*> axes;
    bot->GetCreatureListWithEntryInGrid(axes, NPC_THROW, 20.0f);
    return std::any_of(axes.begin(), axes.end(), [](Creature const* axe)
    {
        return axe && axe->IsAlive();
    });
}
