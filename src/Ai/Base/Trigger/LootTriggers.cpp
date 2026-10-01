/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "LootTriggers.h"
#include "LootObjectStack.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "ServerFacade.h"

bool LootAvailableTrigger::IsActive()
{
    if (!AI_VALUE(bool, "has available loot"))
        return false;

    // Not while the group fights: a priest out of combat itself walked back 40 yd to the last pack's corpses while
    // the tank pulled the next one, and the tank died unhealed (heroic Nexus, run 1939).
    if (TrashCcGroupInCombat(bot))
        return false;

    bool distanceCheck = false;
    if (botAI->HasStrategy("stay", BOT_STATE_NON_COMBAT))
    {
        distanceCheck =
            ServerFacade::instance().IsDistanceLessOrEqualThan(AI_VALUE2(float, "distance", "loot target"), CONTACT_DISTANCE);
    }
    else
    {
        distanceCheck = ServerFacade::instance().IsDistanceLessOrEqualThan(AI_VALUE2(float, "distance", "loot target"),
                                                                 INTERACTION_DISTANCE - 2.0f);
    }

    // Loot target in range, or no hostile targets to deal with first.
    if (distanceCheck || AI_VALUE(GuidVector, "all targets").empty())
        return true;

    // A target that became not loot-possible after being selected - the bot
    // was pulled into combat while approaching it, or it despawned - never
    // returns to range, so without this it stays selected forever and the
    // loot action never gets to pick another. Report active so it can.
    LootObject lootTarget = AI_VALUE(LootObject, "loot target");
    return !lootTarget.IsEmpty() && !lootTarget.IsLootPossible(bot);
}

bool FarFromCurrentLootTrigger::IsActive()
{
    if (TrashCcGroupInCombat(bot))
        return false;

    LootObject loot = AI_VALUE(LootObject, "loot target");
    if (!loot.IsLootPossible(bot))
        return false;

    return AI_VALUE2(float, "distance", "loot target") >= INTERACTION_DISTANCE - 2.0f;
}

bool CanLootTrigger::IsActive() { return AI_VALUE(bool, "can loot"); }
