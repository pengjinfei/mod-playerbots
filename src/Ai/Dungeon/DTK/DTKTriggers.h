/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DTKTRIGGERS_H
#define PLAYERBOTS_DTKTRIGGERS_H

#include "DungeonStrategyUtils.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Trigger.h"

enum DrakTharonIDs
{
    // Trollgore
    NPC_DRAKKARI_INVADER               = 27709,

    // Novos the Summoner
    NPC_NOVOS                          = 26631,
    SPELL_ARCANE_FIELD                 = 47346,
    NPC_CRYSTAL_HANDLER                = 26627,
    NPC_HULKING_CORPSE                 = 27597,
    NPC_RISEN_SHADOWCASTER             = 27600,
    NPC_FETID_TROLL_CORPSE             = 27598,

    // King Dred
    SPELL_GRIEVOUS_BITE                = 48920,  // bleeds every second until the target is at full health

    // The Prophet Tharon'ja
    SPELL_GIFT_OF_THARONJA             = 52509,
};

// Group member with King Dred's Grievous Bite below full health (lowest share of health first), or nullptr.
// The bite only ends on a tick at exactly full health (spell_dred_grievious_bite_aura), and it keeps ticking after the
// fight: a tank bled out waiting at the next pull spot while the healer did nothing (my-mac run100083, run100089).
Player* FindGrievousBiteTarget(Player* bot);

class CorpseExplodeTrigger : public Trigger
{
public:
    CorpseExplodeTrigger(PlayerbotAI* ai) : Trigger(ai, "corpse explode") {}
    bool IsActive() override;
};

class ArcaneFieldTrigger : public Trigger
{
public:
    ArcaneFieldTrigger(PlayerbotAI* ai) : Trigger(ai, "arcane field") {}
    bool IsActive() override;
};

// class CrystalHandlerTrigger : public Trigger
// {
// public:
//     CrystalHandlerTrigger(PlayerbotAI* ai) : Trigger(ai, "crystal handler") {}
//     bool IsActive() override;
// };

class GrievousBiteTrigger : public Trigger
{
public:
    GrievousBiteTrigger(PlayerbotAI* ai) : Trigger(ai, "grievous bite") {}
    bool IsActive() override;
};

class GiftOfTharonjaTrigger : public Trigger
{
public:
    GiftOfTharonjaTrigger(PlayerbotAI* ai) : Trigger(ai, "gift of tharon'ja") {}
    bool IsActive() override;
};

#endif
