/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_LASTMOVEMENTVALUE_H
#define PLAYERBOTS_LASTMOVEMENTVALUE_H

#include "ObjectGuid.h"
#include "TravelNode.h"
#include "Value.h"

class PlayerbotAI;
class Unit;

// High priority movement can override the previous low priority one
enum class MovementPriority
{
    MOVEMENT_IDLE,
    MOVEMENT_WANDER,
    MOVEMENT_NORMAL,
    MOVEMENT_COMBAT,
    MOVEMENT_FORCED
};

// Why a movement was issued. Orthogonal to MovementPriority (which only orders the movement lock):
// it tells the casting code whether the movement may be interrupted so a cast-time spell can go off.
enum class MovementIntent : uint8
{
    TACTICAL,     // default: flee, break contact, dungeon mechanics, anything that did not opt in.
                  // Neither yields to a cast nor interrupts one.
    POSITIONING,  // formation, reaching range, follow, wandering, collision nudges. Yields to a cast.
    SURVIVAL,     // dodge a spike, leave a cone. Behaves like TACTICAL today; reserved so a later
                  // change can let it interrupt a cast in progress.
};

class LastMovement
{
public:
    LastMovement();
    LastMovement(LastMovement& other);

    LastMovement& operator=(LastMovement const& other)
    {
        taxiNodes = other.taxiNodes;
        taxiMaster = other.taxiMaster;
        lastFollow = other.lastFollow;
        lastAreaTrigger = other.lastAreaTrigger;
        lastMoveShort = other.lastMoveShort;
        lastPath = other.lastPath;
        nextTeleport = other.nextTeleport;
        priority = other.priority;
        intent = other.intent;
        issuer = other.issuer;
        return *this;
    };

    void clear();

    void Set(Unit* follow);
    void Set(uint32 mapId, float x, float y, float z, float ori, float delayTime, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL);

    void setShort(WorldPosition point);
    void setPath(TravelPath path);

    std::vector<uint32> taxiNodes;
    ObjectGuid taxiMaster;
    Unit* lastFollow;
    uint32 lastAreaTrigger;
    time_t lastFlee;
    uint32 lastMoveToMapId;
    float lastMoveToX;
    float lastMoveToY;
    float lastMoveToZ;
    float lastMoveToOri;
    float lastdelayTime;
    WorldPosition lastMoveShort;
    uint32 msTime;
    MovementPriority priority;
    MovementIntent intent;
    std::string issuer;  // name of the action that issued the movement; for diagnostics only
    TravelPath lastPath;
    time_t nextTeleport;
    std::future<TravelPath> future;
};

class LastMovementValue : public ManualSetValue<LastMovement&>
{
public:
    LastMovementValue(PlayerbotAI* botAI) : ManualSetValue<LastMovement&>(botAI, data) {}

private:
    LastMovement data = LastMovement();
};

class StayTimeValue : public ManualSetValue<time_t>
{
public:
    StayTimeValue(PlayerbotAI* botAI) : ManualSetValue<time_t>(botAI, 0) {}
};

#endif
