/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DUNGEONRUNVALUES_H
#define PLAYERBOTS_DUNGEONRUNVALUES_H

#include "Value.h"

class PlayerbotAI;

// How far along the dungeon route (yards from the entrance) the leader has walked; only grows. Read by the
// test harness to report progress and detect a stalled run.
class DungeonRunProgressValue : public ManualSetValue<float>
{
public:
    DungeonRunProgressValue(PlayerbotAI* botAI) : ManualSetValue<float>(botAI, 0.0f, "dungeon run progress") {}
};

#endif
