/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DUNGEONROUTEMGR_H
#define PLAYERBOTS_DUNGEONROUTEMGR_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Define.h"

// A dungeon route drafted by tools/route-gen (management repo) and hand-tuned: skeleton nodes in walking order and
// the packs / boss encounters met along them. Loaded once at startup on the world thread; read-only afterwards, so
// map threads may read it without locking.
struct DungeonRouteNode
{
    float along;  // yards from the entrance along the skeleton
    float x, y, z;
};

struct DungeonRouteItem
{
    float along;
    float x, y, z;
    float radius;
    bool side;  // off the path; not cleared unless it joins a fight on its own
    bool boss;
    bool object = false;               // a game object the leader walks to and uses (a lever, a containment sphere)
    bool sent = false;                 // the encounter sends this pack to the group: wait for it, do not pull it
    std::vector<uint32> spawnIds;      // creature spawn ids (packs)
    std::vector<uint32> bossEntries;   // creature entries (boss encounters)
    std::vector<uint32> objectEntries; // game object entries (objects)
    std::vector<uint32> summonEntries; // creature entries within radius of the position (script-summoned packs)
};

struct DungeonRoute
{
    uint32 mapId = 0;
    std::string name;
    std::vector<DungeonRouteNode> nodes;
    std::vector<DungeonRouteItem> items;
};

class DungeonRouteMgr
{
public:
    static DungeonRouteMgr& instance()
    {
        static DungeonRouteMgr instance;
        return instance;
    }

    // Reads every "<mapId>-<name>.route" file in the directory; a map without a file has no route.
    void Load(std::string const& directory);
    DungeonRoute const* Get(uint32 mapId) const;

private:
    DungeonRouteMgr() = default;
    bool LoadFile(std::string const& path, DungeonRoute& route);
    static std::vector<uint32> ParseIdList(std::string_view text);
    // "key=value" field after the coordinates; empty when absent.
    static std::string_view Field(std::vector<std::string_view> const& tokens, std::string_view key);

    std::unordered_map<uint32, DungeonRoute> _routes;
};

#endif
