/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "DungeonRunTriggers.h"

#include "DungeonRouteMgr.h"
#include "Playerbots.h"

bool DungeonRunTrigger::IsActive()
{
    return (!bot->IsInCombat() || bot->getAttackers().empty()) && DungeonRouteMgr::instance().Get(bot->GetMapId());
}
