/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "TOCTriggers.h"
#include "AiObject.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

Creature* ToCFindWalkingChampion(Player* bot, float range)
{
    Creature* nearest = nullptr;
    for (size_t i = 0; i < 10; ++i)  // the ten Grand Champions head availableTargets
    {
        std::list<Creature*> champions;
        bot->GetCreatureListWithEntryInGrid(champions, availableTargets[i], range);
        for (Creature* champion : champions)
        {
            if (!champion->IsAlive() || champion->GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID) ||
                !champion->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE) || champion->HasAura(SPELL_TOC_TRAMPLE_STUN))
                continue;
            // Walking to a new mount (the dismount sets the walk flag); the ground phase has no walk flag.
            if (!champion->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
                continue;
            if (!nearest || bot->GetExactDist2d(champion) < bot->GetExactDist2d(nearest))
                nearest = champion;
        }
    }
    return nearest;
}

bool ToCTrampleChampionTrigger::IsActive()
{
    Unit* vehicleBase = bot->GetVehicleBase();
    if (!vehicleBase || vehicleBase->GetEntry() != NPC_ARGENT_WARHORSE)
        return false;

    return ToCFindWalkingChampion(bot, 80.0f) != nullptr;
}

bool ToCLanceTrigger::IsActive()
{
    if (bot->GetVehicle())
        return false;

    Unit* mount1 = bot->FindNearestCreature(NPC_ARGENT_WARHORSE, 100.0f);
    if (!mount1)
        return false;

    Unit* mount2 = bot->FindNearestCreature(NPC_ARGENT_BATTLEWORG, 100.0f);
    if (!mount2)
        return false;

    if (!bot->HasItemOrGemWithIdEquipped(ITEM_LANCE, 1))
        return true;

    if (bot->HasItemCount(ITEM_LANCE, 1))
        return false;

    // Find the nearest spear
    GameObject* lanceRack = bot->FindNearestGameObject(OBJECT_LANCE_RACK, 100.0f);
    if (!lanceRack)
        return false;

    return true;
}

bool ToCUELanceTrigger::IsActive()
{
    if (bot->GetVehicle())
        return false;

    Unit* mount1 = bot->FindNearestCreature(NPC_ARGENT_WARHORSE, 100.0f);
    if (!mount1 && bot->HasItemOrGemWithIdEquipped(ITEM_LANCE, 1))
        return true;

    Unit* mount2 = bot->FindNearestCreature(NPC_ARGENT_BATTLEWORG, 100.0f);
    if (!mount2 && bot->HasItemOrGemWithIdEquipped(ITEM_LANCE, 1))
        return true;

    return false;
}

bool ToCMountedTrigger::IsActive()
{
    Unit* vehicleBase = bot->GetVehicleBase();
    Vehicle* vehicle = bot->GetVehicle();
    if (!vehicleBase || !vehicle)
        return false;

    uint32 entry = vehicleBase->GetEntry();
    return entry == NPC_ARGENT_BATTLEWORG || entry == NPC_ARGENT_WARHORSE;
}

bool ToCMountNearTrigger::IsActive()
{
    if (bot->GetVehicle())
        return false;

    Unit* mount1 = bot->FindNearestCreature(NPC_ARGENT_WARHORSE, 100.0f);
    if (!mount1)
        return false;

    Unit* mount2 = bot->FindNearestCreature(NPC_ARGENT_BATTLEWORG, 100.0f);
    if (!mount2)
        return false;

    return true;
}

bool ToCEadricTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "eadric the pure");
    if (!boss)
        return false;

    return true;
}

bool ToCPaletressShieldTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "argent confessor paletress");
    if (!boss || !boss->HasAura(SPELL_PALETRESS_REFLECTIVE_SHIELD))
        return false;

    // The shield only breaks by absorbing damage, so holding off her for good lets Renew heal her back to full
    // (run1223: 23% -> 100%). Stay off her only while her Memory is alive, then break the shield.
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit != boss && unit->IsAlive() && unit->IsInCombat())
            return true;
    }
    return false;
}
