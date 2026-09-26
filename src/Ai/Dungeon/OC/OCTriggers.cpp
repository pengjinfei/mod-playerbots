/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "OCTriggers.h"
#include "AiObject.h"
#include "AiObjectContext.h"
#include "Playerbots.h"
#include "Unit.h"

bool DrakosUnstableSphereTrigger::IsActive()
{
    // Doesn't seem to be much point trying to get melee to dodge this,
    // they get hit anyway and it just causes a lot of running around and chaos
    // if (botAI->IsMelee(bot)) { return false; }
    if (botAI->IsTank(bot)) { return false; }

    GuidVector targets = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->GetEntry() == NPC_UNSTABLE_SPHERE)
        {
            return true;
        }
    }
    return false;
}

Unit* OccMasterlessDrakeTarget(Player* bot)
{
    if (bot->GetMapId() != OCULUS_MAP_ID)
        return nullptr;

    Creature* eregos = bot->FindNearestCreature(NPC_LEY_GUARDIAN_EREGOS, 250.0f, true);
    if (!eregos || eregos->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE))
        return nullptr;

    return eregos;
}

bool DrakeMountTrigger::IsActive()
{
    Player* master = botAI->GetMaster();
    if (!master)
        return !bot->GetVehicleBase() && OccMasterlessDrakeTarget(bot);

    return master->GetVehicleBase() && !bot->GetVehicleBase();
}

bool DrakeDismountTrigger::IsActive()
{
    Player* master = botAI->GetMaster();
    if (!master) { return false; }

    return !master->GetVehicleBase() && bot->GetVehicleBase();
}

bool GroupFlyingTrigger::IsActive()
{
    Player* master = botAI->GetMaster();
    if (!master)
        return bot->GetMapId() == OCULUS_MAP_ID && bot->GetVehicleBase();

    return master->GetVehicleBase() && bot->GetVehicleBase();
}

bool DrakeCombatTrigger::IsActive()
{
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    if (!targets.empty())
        return true;

    // Masterless parties have nobody to start the fight from a drake: open on Eregos ourselves.
    return !botAI->GetMaster() && bot->GetVehicleBase() && OccMasterlessDrakeTarget(bot);
}

bool VarosCloudstriderTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "varos cloudstrider");
    if (!boss) { return false; }

    return true;
}

bool UromArcaneExplosionTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "mage-lord urom");
    if (!boss) { return false; }

    return bool(boss->FindCurrentSpellBySpellId(SPELL_EMPOWERED_ARCANE_EXPLOSION));
}

bool UromTimeBombTrigger::IsActive()
{
    return bot->HasAura(SPELL_TIME_BOMB);
}
