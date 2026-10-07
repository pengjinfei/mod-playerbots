/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_VHTRIGGERS_H
#define PLAYERBOTS_VHTRIGGERS_H

#include "DungeonStrategyUtils.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Trigger.h"

enum VioletHoldIDs
{
    // Ichoron
    SPELL_DRAINED                      = 59820,
    NPC_ICHOR_GLOBULE                  = 29321,

    // Zuramat the Obliterator
    SPELL_VOID_SHIFTED                 = 54343,
    SPELL_SHROUD_OF_DARKNESS_N         = 54524,
    SPELL_SHROUD_OF_DARKNESS_H         = 59745,
    NPC_VOID_SENTRY                    = 29364,

    // Portals
    NPC_PORTAL_GUARDIAN                = 30660,
    NPC_PORTAL_KEEPER_1                = 30695,
    NPC_PORTAL_KEEPER_2                = 30893,
};

#define SPELL_SHROUD_OF_DARKNESS    DUNGEON_MODE(bot, SPELL_SHROUD_OF_DARKNESS_N, SPELL_SHROUD_OF_DARKNESS_H)

class ErekemTargetTrigger : public Trigger
{
public:
    ErekemTargetTrigger(PlayerbotAI* ai) : Trigger(ai, "erekem target") {}
    bool IsActive() override;
};

class IchoronTargetTrigger : public Trigger
{
public:
    IchoronTargetTrigger(PlayerbotAI* ai) : Trigger(ai, "ichoron target") {}
    bool IsActive() override;
};

class VoidShiftTrigger : public Trigger
{
public:
    VoidShiftTrigger(PlayerbotAI* ai) : Trigger(ai, "void shift") {}
    bool IsActive() override;
};

class ShroudOfDarknessTrigger : public Trigger
{
public:
    ShroudOfDarknessTrigger(PlayerbotAI* ai) : Trigger(ai, "shroud of darkness") {}
    bool IsActive() override;
};

// A portal held open by its Guardian or Keeper sends three or four Azure invaders every 20 s until that one dies. The
// group fought the invaders as they came and never reached the keeper; they piled up over waves 7-11 and the group
// died (Violet Hold full run 2303). Damage dealers kill the keeper, the tank holds the invaders.
Creature* VioletHoldPortalKeeper(Player* bot);

class PortalKeeperTrigger : public Trigger
{
public:
    PortalKeeperTrigger(PlayerbotAI* ai) : Trigger(ai, "portal keeper") {}
    bool IsActive() override;
};

class CyanigosaPositioningTrigger : public Trigger
{
public:
    CyanigosaPositioningTrigger(PlayerbotAI* ai) : Trigger(ai, "cyanigosa positioning") {}
    bool IsActive() override;
};

#endif
