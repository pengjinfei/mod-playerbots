/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UPTriggers.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

bool SkadiFreezingCloudTrigger::IsActive()
{
    Unit* bossMount = AI_VALUE2(Unit*, "find target", "grauf");
    if (!bossMount) { return false; }

    // Need to check two conditions here - the persistent ground effect doesn't
    // seem to be detectable until 3-5 secs in, despite it dealing damage.
    // The initial breath triggers straight away but once it's over, the bots will run back on
    // to the freezing cloud and take damage.
    // Therefore check both conditions and trigger on either.

    // Check this one first, if true then we don't need to iterate over any objects
    if (bossMount->HasAura(SPELL_FREEZING_CLOUD_BREATH_RIGHT) || bossMount->HasAura(SPELL_FREEZING_CLOUD_BREATH_LEFT))
    {
        return true;
    }

    // Otherwise, check for persistent ground objects emitting the freezing cloud
    GuidVector objects = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto i = objects.begin(); i != objects.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_BREATH_TRIGGER)
        {
            Unit::AuraApplicationMap const& Auras = unit->GetAppliedAuras();
            for (Unit::AuraApplicationMap::const_iterator itr = Auras.begin(); itr != Auras.end(); ++itr)
            {
                Aura* aura = itr->second->GetBase();
                if (aura && aura->GetId() == SPELL_FREEZING_CLOUD)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

float SkadiBreathSafeY(Player* bot)
{
    Creature* grauf = bot->FindNearestCreature(NPC_GRAUF, 250.0f, true);
    if (!grauf)
        return 0.0f;

    if (grauf->HasAura(SPELL_FREEZING_CLOUD_BREATH_LEFT))
        return SKADI_BREATH_SPLIT_Y - 5.0f;
    if (grauf->HasAura(SPELL_FREEZING_CLOUD_BREATH_RIGHT))
        return SKADI_BREATH_SPLIT_Y + 5.0f;
    return 0.0f;
}

bool SkadiBreathSideTrigger::IsActive()
{
    float const safeY = SkadiBreathSafeY(bot);
    if (!safeY)
        return false;

    // Already on the safe half, with a yard of margin past the split.
    return safeY < SKADI_BREATH_SPLIT_Y ? bot->GetPositionY() > SKADI_BREATH_SPLIT_Y - 1.0f
                                        : bot->GetPositionY() < SKADI_BREATH_SPLIT_Y + 1.0f;
}

bool SkadiWhirlwindTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "skadi the ruthless");
    return boss && boss->HasAura(SPELL_SKADI_WHIRLWIND);
}

bool YmironBaneTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "king ymiron");
    if (!boss) { return false; }

    return boss->FindCurrentSpellBySpellId(SPELL_BANE) || boss->HasAura(SPELL_BANE);
}

Unit* SkadiNextGauntletAdd(Player* bot)
{
    Unit* nearest = nullptr;
    for (uint32 entry : { NPC_YMIRJAR_WARRIOR, NPC_YMIRJAR_WITCH_DOCTOR, NPC_YMIRJAR_HARPOONER })
    {
        std::list<Creature*> adds;
        bot->GetCreatureListWithEntryInGrid(adds, entry, 150.0f);
        for (Creature* add : adds)
        {
            if (!add->IsAlive() || !add->IsHostileTo(bot) || !add->isTargetableForAttack(true, bot))
                continue;
            if (!nearest || bot->GetExactDist2d(add) < bot->GetExactDist2d(nearest))
                nearest = add;
        }
    }
    return nearest;
}

Unit* SkadiOnGround(Player* bot)
{
    Creature* skadi = bot->FindNearestCreature(NPC_SKADI_THE_RUTHLESS, 150.0f, true);
    if (!skadi || skadi->GetVehicle() || !skadi->IsInCombat() || !skadi->isTargetableForAttack(true, bot))
        return nullptr;
    return skadi;
}

// Held at the Harpoon Launchers, the group has Skadi land among it when Grauf falls: the tank was still out of combat
// and he whirlwinded the healer to death in 30 s, the wipe followed (my-mac run 100215).
bool SkadiLandedTrigger::IsActive()
{
    if (!botAI->IsTank(bot))
        return false;
    Unit* skadi = SkadiOnGround(bot);
    return skadi && AI_VALUE(Unit*, "current target") != skadi;
}

bool SkadiTankPullNextTrigger::IsActive()
{
    if (!botAI->IsTank(bot) || !bot->FindNearestCreature(NPC_GRAUF, 250.0f, true))
        return false;

    // Busy with a pack already: finish it first.
    if (!AI_VALUE(GuidVector, "attackers").empty())
        return false;

    return SkadiNextGauntletAdd(bot) != nullptr;
}

bool SkadiHarpoonPickupTrigger::IsActive()
{
    // Damage dealers fetch harpoons; the tank holds the gauntlet adds and the healer stays on the group.
    if (botAI->IsTank(bot) || botAI->IsHeal(bot) || bot->HasItemCount(ITEM_HARPOON, 1))
        return false;

    if (!bot->FindNearestCreature(NPC_GRAUF, 250.0f, true))
        return false;

    GameObject* harpoon = bot->FindNearestGameObject(GO_HARPOON, 60.0f);
    return harpoon && harpoon->isSpawned();
}

bool SkadiHarpoonLaunchTrigger::IsActive()
{
    return bot->HasItemCount(ITEM_HARPOON, 1) && bot->FindNearestCreature(NPC_GRAUF, 250.0f, true);
}
