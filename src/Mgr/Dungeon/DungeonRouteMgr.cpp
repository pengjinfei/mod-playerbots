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

bool DungeonRouteMgr::ParsePoint(std::string_view text, float& x, float& y, float& z)
{
    std::vector<std::string_view> parts = Acore::Tokenize(text, ',', false);
    if (parts.size() != 3)
        return false;
    Optional<float> px = Acore::StringTo<float>(parts[0]);
    Optional<float> py = Acore::StringTo<float>(parts[1]);
    Optional<float> pz = Acore::StringTo<float>(parts[2]);
    if (!px || !py || !pz)
        return false;
    x = *px;
    y = *py;
    z = *pz;
    return true;
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
        if (tokens[0] == "pass")
        {
            DungeonRoutePass pass;
            pass.from = *along;
            pass.to = Acore::StringTo<float>(Field(tokens, "to")).value_or(*along);
            pass.entries = ParseIdList(Field(tokens, "entry"));
            route.passes.push_back(pass);
            continue;
        }
        if (tokens[0] != "pack" && tokens[0] != "boss" && tokens[0] != "object" && tokens[0] != "summoned" &&
            tokens[0] != "drop" && tokens[0] != "cross" && tokens[0] != "wait")
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
        item.noCc = Field(tokens, "cc") == "0";
        if (std::string_view const away = Field(tokens, "away"); !away.empty())
        {
            size_t const colon = away.find(':');
            if (colon != std::string_view::npos)
            {
                item.awaySpawn = Acore::StringTo<uint32>(away.substr(0, colon)).value_or(0);
                item.awayDistance = Acore::StringTo<float>(away.substr(colon + 1)).value_or(0.0f);
            }
        }
        if (std::string_view const clear = Field(tokens, "clear"); !clear.empty())
        {
            size_t const colon = clear.find(':');
            if (colon != std::string_view::npos)
            {
                item.clearSpawn = Acore::StringTo<uint32>(clear.substr(0, colon)).value_or(0);
                item.clearDistance = Acore::StringTo<float>(clear.substr(colon + 1)).value_or(0.0f);
            }
        }
        item.pullDistance = Acore::StringTo<float>(Field(tokens, "pull")).value_or(0.0f);
        item.spawnIds = ParseIdList(Field(tokens, "spawns"));
        item.object = tokens[0] == "object";
        item.drop = tokens[0] == "drop";
        if (tokens[0] == "wait")
            item.waitMs = Acore::StringTo<uint32>(Field(tokens, "ms")).value_or(0);
        // rim=<x>,<y>,<z>: the navmesh has no floor over a hole, so a drop's approach aims at its edge.
        if (item.drop && !ParsePoint(Field(tokens, "rim"), item.rimX, item.rimY, item.rimZ))
        {
            LOG_ERROR("server.loading", "Dungeon routes: '{}' line {}: drop without rim=<x>,<y>,<z>", path,
                      lineNumber);
            return false;
        }
        item.hold = ParsePoint(Field(tokens, "hold"), item.holdX, item.holdY, item.holdZ);
        item.from = ParsePoint(Field(tokens, "from"), item.fromX, item.fromY, item.fromZ);
        item.cross = tokens[0] == "cross";
        // from=<x>,<y>,<z>: where the walkway starts, on the navmesh; the group gathers there and walks over together.
        if (item.cross && !item.from)
        {
            LOG_ERROR("server.loading", "Dungeon routes: '{}' line {}: cross without from=<x>,<y>,<z>", path,
                      lineNumber);
            return false;
        }
        bool const summoned = tokens[0] == "summoned";
        (item.object ? item.objectEntries : summoned ? item.summonEntries : item.bossEntries) =
            ParseIdList(Field(tokens, "entry"));
        if (summoned && item.radius <= 0.0f)
            item.radius = 20.0f;
        if (!item.drop && !item.cross && !item.waitMs && item.spawnIds.empty() && item.bossEntries.empty() &&
            item.objectEntries.empty() && item.summonEntries.empty())
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
