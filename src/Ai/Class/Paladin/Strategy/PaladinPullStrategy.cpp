/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "PaladinPullStrategy.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"

std::string PaladinPullStrategy::GetPullActionName() const
{
    Unit* target = GetTarget();
    if (!target ||
        (!botAI->HasStrategy("tank", BOT_STATE_COMBAT) && !botAI->HasStrategy("tank", BOT_STATE_NON_COMBAT)))
    {
        return PullStrategy::GetPullActionName();
    }

    // The shield bounces, and crowd-control protection holds it while a crowd-control mark stands within 45 yd:
    // the pull then never went out and the tank walked into the pack (Ahn'kahet pack 1182, realm 8 run 2463).
    bool markedNear = false;
    for (uint8 icon : { TRASH_CC_ICON_TRIANGLE, TRASH_CC_ICON_MOON, TRASH_CC_ICON_SQUARE, TRASH_CC_ICON_CROSS })
        if (Unit* marked = TrashCcIconUnit(botAI, icon))
            markedNear = markedNear ||
                         (marked != target && marked->IsAlive() && botAI->GetBot()->GetDistance(marked) <= 45.0f);
    if (!markedNear && botAI->CanCastSpell("avenger's shield", target))
        return "avenger's shield";

    if (botAI->CanCastSpell("hand of reckoning", target))
        return "hand of reckoning";

    return PullStrategy::GetPullActionName();
}

std::string PaladinPullStrategy::GetPreActionName() const
{
    if (botAI->HasStrategy("tank", BOT_STATE_COMBAT) || botAI->HasStrategy("tank", BOT_STATE_NON_COMBAT))
        return "";

    return PullStrategy::GetPreActionName();
}
