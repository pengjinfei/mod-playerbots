/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"
#include "NaxxBossHelper.h"
#include "NaxxSpellIds.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

bool SapphironGroundPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    if (botAI->IsMainTank(bot))
    {
        if (AI_VALUE2(bool, "has aggro", "current target"))
            return MoveTo(NAXX_MAP_ID, helper.mainTankPos.first, helper.mainTankPos.second, helper.GENERIC_HEIGHT, false, false, false,
                          false, MovementPriority::MOVEMENT_COMBAT);

        return false;
    }
    if (helper.JustLanded())
    {
        uint32 index = botAI->GetGroupSlotIndex(bot);
        float start_angle = 0.85 * M_PI;
        float offset_angle = M_PI * 0.02 * index;
        float angle = start_angle + offset_angle;
        float distance;
        if (botAI->IsRanged(bot))
            distance = 35.0f;
        else if (botAI->IsHeal(bot))
            distance = 30.0f;
        else
            distance = 5.0f;

        float posX = helper.center.first + cos(angle) * distance;
        float posY = helper.center.second + sin(angle) * distance;
        if (MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
            return true;

        return MoveInside(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, 2.0f, MovementPriority::MOVEMENT_COMBAT);
    }
    else
    {
        std::vector<float> dest;
        if (helper.FindPosToAvoidChill(dest))
            return MoveTo(NAXX_MAP_ID, dest[0], dest[1], dest[2], false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool SapphironFlightPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    if (helper.WaitForExplosion())
        return MoveToNearestIcebolt();

    std::vector<float> dest;
    if (helper.FindPosToAvoidChill(dest))
        return MoveTo(NAXX_MAP_ID, dest[0], dest[1], dest[2], false, false, false, false, MovementPriority::MOVEMENT_COMBAT);

    // Spread before the Icebolts land: each one also hits everyone within 10 yd of its target, and the raid 10 stood
    // stacked in the air phase - both Icebolts hit all ten for ~6k each, and Frost Aura then took the low ones down
    // before the breath (run 1615: four of the six air-phase deaths were Frost Aura ticks). One spot per group slot on
    // a 22 yd ring round the room centre, ~12 yd apart for ten.
    uint32 const index = botAI->GetGroupSlotIndex(bot);
    float const angle = 2.0f * M_PI * float(index % 10) / 10.0f;
    float const posX = helper.center.first + cos(angle) * 22.0f;
    float const posY = helper.center.second + sin(angle) * 22.0f;
    if (bot->GetExactDist2d(posX, posY) <= 2.0f)
        return false;
    return MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT);
}

bool SapphironFlightPositionAction::MoveToNearestIcebolt()
{
    float posX = 0.0f, posY = 0.0f;
    if (!helper.FindHideSpot(posX, posY) || bot->GetExactDist2d(posX, posY) <= 1.0f)
        return false;
    // No MoveNear fallback: MoveTo also returns false while the previous move is still under way, and the fallback then
    // sent the bot to any spot 3 yd from the block, usually not behind it - the Frost Breath victims stood 2-6 yd from
    // a block on the dragon's side (raid 10 runs 1678/1679). The boss is the cached one: in the air Sapphiron is not on
    // the bots' threatened-by lists, so "find target" came back empty.
    return MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT);
}
