/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_BOTAREATRIGGERINDEX_H
#define PLAYERBOTS_BOTAREATRIGGERINDEX_H

#include <unordered_map>
#include <vector>

#include "Define.h"

// Area triggers a bot should report the way a game client does (CMSG_AREATRIGGER on entering the box): the scripted
// and quest triggers of dungeon and raid maps. Teleport and tavern triggers are left out - a bot never leaves an
// instance by walking over its exit. Built once on the world thread at startup; read-only afterwards.
class BotAreaTriggerIndex
{
public:
    static BotAreaTriggerIndex& instance()
    {
        static BotAreaTriggerIndex instance;
        return instance;
    }

    void Build();
    std::vector<uint32> const* ForMap(uint32 mapId) const;

private:
    BotAreaTriggerIndex() = default;

    std::unordered_map<uint32, std::vector<uint32>> _byMap;
};

#endif
