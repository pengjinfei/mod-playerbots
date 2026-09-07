/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UKTriggers.h"
#include "AiObjectContext.h"
#include "Creature.h"
#include "Playerbots.h"

#include <algorithm>
#include <list>

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

    if (boss->FindCurrentSpellBySpellId(SPELL_SMASH) ||
        boss->FindCurrentSpellBySpellId(SPELL_DARK_SMASH))
        {
            return true;
        }
    return false;
}

bool NotBehindIngvarTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    if (!boss || currentTarget != boss || botAI->IsTank(bot) || botAI->IsHeal(bot)) { return false; }

    return !AI_VALUE2(bool, "behind", "current target");
}

bool IngvarShadowAxeTrigger::IsActive()
{
    if (botAI->IsTank(bot)) { return false; }

    std::list<Creature*> axes;
    bot->GetCreatureListWithEntryInGrid(axes, NPC_THROW, 20.0f);
    return std::any_of(axes.begin(), axes.end(), [](Creature const* axe)
    {
        return axe && axe->IsAlive();
    });
}
