/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "DungeonRouteMgr.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "Log.h"
#include "StringConvert.h"
#include "Tokenize.h"

std::vector<uint32> DungeonRouteMgr::ParseIdList(std::string_view text)
{
    std::vector<uint32> ids;
    for (std::string_view token : Acore::Tokenize(text, ',', false))
        if (Optional<uint32> id = Acore::StringTo<uint32>(token))
            ids.push_back(*id);
    return ids;
}

std::string_view DungeonRouteMgr::Field(std::vector<std::string_view> const& tokens, std::string_view key)
{
    for (std::string_view token : tokens)
        if (token.size() > key.size() && token.substr(0, key.size()) == key && token[key.size()] == '=')
            return token.substr(key.size() + 1);
    return {};
}

void DungeonRouteMgr::Load(std::string const& directory)
{
    _routes.clear();
    std::error_code error;
    if (directory.empty() || !std::filesystem::is_directory(directory, error))
    {
        LOG_INFO("server.loading", "Dungeon routes: directory '{}' not found, no routes loaded", directory);
        return;
    }

    for (auto const& entry : std::filesystem::directory_iterator(directory, error))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".route")
            continue;
        std::string const stem = entry.path().stem().string();
        Optional<uint32> mapId = Acore::StringTo<uint32>(stem.substr(0, stem.find('-')));
        if (!mapId)
        {
            LOG_ERROR("server.loading", "Dungeon routes: '{}' does not start with a map id", entry.path().string());
            continue;
        }
        DungeonRoute route;
        route.mapId = *mapId;
        route.name = stem;
        if (!LoadFile(entry.path().string(), route))
            continue;
        LOG_INFO("server.loading", "Dungeon routes: map {} '{}' - {} nodes, {} packs/encounters", route.mapId,
                 route.name, route.nodes.size(), route.items.size());
        _routes[route.mapId] = std::move(route);
    }
}

bool DungeonRouteMgr::LoadFile(std::string const& path, DungeonRoute& route)
{
    std::ifstream file(path);
    if (!file)
    {
        LOG_ERROR("server.loading", "Dungeon routes: cannot open '{}'", path);
        return false;
    }

    std::string line;
    uint32 lineNumber = 0;
    while (std::getline(file, line))
    {
        ++lineNumber;
        std::string_view text = line;
        text = text.substr(0, text.find('#'));
        std::vector<std::string_view> tokens = Acore::Tokenize(text, ' ', false);
        if (tokens.empty())
            continue;
        if (tokens.size() < 5)
        {
            LOG_ERROR("server.loading", "Dungeon routes: '{}' line {}: expected '<kind> <along> <x> <y> <z> ...'", path,
                      lineNumber);
            return false;
        }
        Optional<float> along = Acore::StringTo<float>(tokens[1]);
        Optional<float> x = Acore::StringTo<float>(tokens[2]);
        Optional<float> y = Acore::StringTo<float>(tokens[3]);
        Optional<float> z = Acore::StringTo<float>(tokens[4]);
        if (!along || !x || !y || !z)
        {
            LOG_ERROR("server.loading", "Dungeon routes: '{}' line {}: bad number", path, lineNumber);
            return false;
        }
        if (tokens[0] == "node")
        {
            route.nodes.push_back({*along, *x, *y, *z});
            continue;
        }
        if (tokens[0] != "pack" && tokens[0] != "boss" && tokens[0] != "object")
        {
            LOG_ERROR("server.loading", "Dungeon routes: '{}' line {}: unknown kind '{}'", path, lineNumber, tokens[0]);
            return false;
        }
        DungeonRouteItem item;
        item.along = *along;
        item.x = *x;
        item.y = *y;
        item.z = *z;
        item.boss = tokens[0] == "boss";
        item.radius = Acore::StringTo<float>(Field(tokens, "radius")).value_or(0.0f);
        item.side = Field(tokens, "side") == "1";
        item.sent = Field(tokens, "sent") == "1";
        item.spawnIds = ParseIdList(Field(tokens, "spawns"));
        item.object = tokens[0] == "object";
        (item.object ? item.objectEntries : item.bossEntries) = ParseIdList(Field(tokens, "entry"));
        if (item.spawnIds.empty() && item.bossEntries.empty() && item.objectEntries.empty())
        {
            LOG_ERROR("server.loading", "Dungeon routes: '{}' line {}: pack without spawns= / boss or object without "
                      "entry=", path, lineNumber);
            return false;
        }
        route.items.push_back(std::move(item));
    }

    auto byAlong = [](auto const& a, auto const& b) { return a.along < b.along; };
    std::stable_sort(route.nodes.begin(), route.nodes.end(), byAlong);
    std::stable_sort(route.items.begin(), route.items.end(), byAlong);
    if (route.nodes.empty())
    {
        LOG_ERROR("server.loading", "Dungeon routes: '{}' has no nodes", path);
        return false;
    }
    return true;
}

DungeonRoute const* DungeonRouteMgr::Get(uint32 mapId) const
{
    auto const itr = _routes.find(mapId);
    return itr != _routes.end() ? &itr->second : nullptr;
}
