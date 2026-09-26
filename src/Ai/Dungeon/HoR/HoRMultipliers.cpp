/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRMultipliers.h"
#include "HoRTriggers.h"
#include "Playerbots.h"

float HoRHoldHeroismForBossMultiplier::GetValue(Action* action)
{
    std::string const name = action->getName();
    if (name != "heroism" && name != "bloodlust")
        return 1.0f;

    if (!bot->IsInCombat())
        return 1.0f;

    // Falric runs 4 waves of spirits (~230 s) before he engages: Heroism cast on the first wave was gone by
    // then, and the wipes ended with him at 0-2% under Hopelessness.
    return GetHoRWaveBossState(bot) == HoRWaveBossState::Waiting ? 0.0f : 1.0f;
}
