/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DUNGEONRUNTRIGGERS_H
#define PLAYERBOTS_DUNGEONRUNTRIGGERS_H

#include "Trigger.h"

class PlayerbotAI;

// Out of combat on a map that has a dungeon route (one hash lookup).
class DungeonRunTrigger : public Trigger
{
public:
    DungeonRunTrigger(PlayerbotAI* botAI) : Trigger(botAI, "dungeon run") {}

    bool IsActive() override;
};

class DungeonRunGroupHeldTrigger : public Trigger
{
public:
    DungeonRunGroupHeldTrigger(PlayerbotAI* botAI) : Trigger(botAI, "dungeon run group held") {}

    bool IsActive() override;
};

#endif
