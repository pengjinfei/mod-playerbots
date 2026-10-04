/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "DungeonRunTriggers.h"

#include "DungeonRouteMgr.h"
#include "DungeonRunActions.h"
#include "Playerbots.h"

bool DungeonRunTrigger::IsActive()
{
    DungeonRoute const* route = DungeonRouteMgr::instance().Get(bot->GetMapId());
    if (!route)
        return false;
    if (!bot->IsInCombat() || bot->getAttackers().empty())
        return true;
    // Walking on through a pass stretch, fighting on the way: what attacks there keeps coming back.
    DungeonRoutePass const* pass = route->PassAt(AI_VALUE(float, "dungeon run progress"));
    return pass && OnlyPassAttackers(*pass, bot);
}

bool DungeonRunGroupHeldTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;
    auto* advance = dynamic_cast<DungeonRunAdvanceAction*>(context->GetAction("dungeon run advance"));
    return advance && advance->HoldsGroup();
}
