/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "BotAreaTriggerIndex.h"

#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "QueryResult.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "DBCEnums.h"

void BotAreaTriggerIndex::Build()
{
    _byMap.clear();
    // ObjectMgr keeps the `areatrigger` rows but has no iterator; read the ids once here (world thread, startup).
    QueryResult result = WorldDatabase.Query("SELECT entry, map FROM areatrigger");
    if (!result)
        return;

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 const id = fields[0].Get<uint32>();
        uint32 const mapId = fields[1].Get<uint32>();
        MapEntry const* map = sMapStore.LookupEntry(mapId);
        if (!map || !map->IsDungeon())
            continue;
        if (!sObjectMgr->GetAreaTrigger(id) || sObjectMgr->GetAreaTriggerTeleport(id) ||
            sObjectMgr->IsTavernAreaTrigger(id, FACTION_MASK_ALLIANCE) ||
            sObjectMgr->IsTavernAreaTrigger(id, FACTION_MASK_HORDE))
            continue;
        _byMap[mapId].push_back(id);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", "Bot area triggers: {} scripted/quest triggers on {} dungeon/raid maps", count,
             _byMap.size());
}

std::vector<uint32> const* BotAreaTriggerIndex::ForMap(uint32 mapId) const
{
    auto const itr = _byMap.find(mapId);
    return itr != _byMap.end() ? &itr->second : nullptr;
}
