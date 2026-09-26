/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_UPTRIGGERS_H
#define PLAYERBOTS_UPTRIGGERS_H

#include "DungeonStrategyUtils.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Trigger.h"

enum UtgardePinnacleIDs
{
    // Skadi the Ruthless
    SPELL_FREEZING_CLOUD_N          = 47579,
    SPELL_FREEZING_CLOUD_H          = 60020,
    SPELL_FREEZING_CLOUD_BREATH     = 47592,
    NPC_BREATH_TRIGGER              = 28351,
    NPC_GRAUF                       = 26893,
    // Harpoon chain: a dead Ymirjar Harpooner leaves GO_HARPOON; using it gives ITEM_HARPOON, which opens a
    // Harpoon Launcher (lock 1777 needs the item). Three launches while Grauf hovers at the east end bring him down.
    GO_HARPOON                      = 192539,
    ITEM_HARPOON                    = 37372,
    GO_HARPOON_LAUNCHER_1           = 192175,
    GO_HARPOON_LAUNCHER_2           = 192176,
    GO_HARPOON_LAUNCHER_3           = 192177,
    SPELL_SKADI_WHIRLWIND_N         = 50228,
    SPELL_SKADI_WHIRLWIND_H         = 59322,

    // King Ymiron
    SPELL_BANE_N                    = 48294,
    SPELL_BANE_H                    = 59301,
};

#define SPELL_FREEZING_CLOUD        DUNGEON_MODE(bot, SPELL_FREEZING_CLOUD_N, SPELL_FREEZING_CLOUD_H)
#define SPELL_SKADI_WHIRLWIND       DUNGEON_MODE(bot, SPELL_SKADI_WHIRLWIND_N, SPELL_SKADI_WHIRLWIND_H)
#define SPELL_BANE                  DUNGEON_MODE(bot, SPELL_BANE_N, SPELL_BANE_H)

// const float SKADI_BREATH_CENTRELINE = -512.46875f;

class SkadiFreezingCloudTrigger : public Trigger
{
public:
    SkadiFreezingCloudTrigger(PlayerbotAI* ai) : Trigger(ai, "skadi freezing cloud") {}
    bool IsActive() override;
};

class SkadiWhirlwindTrigger : public Trigger
{
public:
    SkadiWhirlwindTrigger(PlayerbotAI* ai) : Trigger(ai, "skadi whirlwind") {}
    bool IsActive() override;
};

// Grauf's hover point at the east end of the gauntlet, inside all three launchers' 60 yd cone.
const Position GRAUF_HOVER_POSITION = Position(521.9f, -545.3f, 117.4f);

class SkadiHarpoonPickupTrigger : public Trigger
{
public:
    SkadiHarpoonPickupTrigger(PlayerbotAI* ai) : Trigger(ai, "skadi harpoon pickup") {}
    bool IsActive() override;
};

class SkadiHarpoonLaunchTrigger : public Trigger
{
public:
    SkadiHarpoonLaunchTrigger(PlayerbotAI* ai) : Trigger(ai, "skadi harpoon launch") {}
    bool IsActive() override;
};

class YmironBaneTrigger : public Trigger
{
public:
    YmironBaneTrigger(PlayerbotAI* ai) : Trigger(ai, "ymiron bane") {}
    bool IsActive() override;
};

#endif
