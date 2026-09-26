/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoRActions.h"
#include "HoRTriggers.h"
#include "Playerbots.h"

bool HoREscapeKeepUpAction::Execute(Event /*event*/)
{
    HoREscape const escape = GetHoREscape(bot);
    if (!escape.leader)
        return false;

    // Gather a few yards around the leader, spread by group slot.
    float const angle = escape.leader->GetOrientation() + float(botAI->GetGroupSlotIndex(bot)) * 1.256637f;
    float const x = escape.leader->GetPositionX() + 5.0f * std::cos(angle);
    float const y = escape.leader->GetPositionY() + 5.0f * std::sin(angle);
    if (!sPlayerbotAIConfig.logInGroupOnly)
        LOG_DEBUG("playerbots", "hor-escape bot={} leaderDist={:.1f} lkBehind={:.1f}", bot->GetName(),
                  bot->GetExactDist2d(escape.leader),
                  (bot->GetPositionX() - escape.lichKing->GetPositionX()) +
                      (bot->GetPositionY() - escape.lichKing->GetPositionY()));
    return MoveTo(bot->GetMapId(), x, y, escape.leader->GetPositionZ(), false, false, false, true,
                  MovementPriority::MOVEMENT_COMBAT);
}
