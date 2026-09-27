/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ROGUETRIGGERS_H
#define PLAYERBOTS_ROGUETRIGGERS_H

#include "GenericTriggers.h"

class Player;
class PlayerbotAI;

class KickInterruptSpellTrigger : public InterruptSpellTrigger
{
public:
    KickInterruptSpellTrigger(PlayerbotAI* botAI) : InterruptSpellTrigger(botAI, "kick") {}
};

class SliceAndDiceTrigger : public BuffTrigger
{
public:
    SliceAndDiceTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "slice and dice") {}
};

// The Hunger for Blood buff on the rogue (63848) is missing or about to run out, and the rogue knows the spell.
bool RogueHungerForBloodNeedsRefresh(Player* bot);

// Hunger for Blood needs a bleed on the target. Anyone's bleed will do (a warrior's Deep Wounds or Rend, a feral
// druid, another rogue); only without one does the rogue open with a short Rupture of his own.
class HungerForBloodTrigger : public Trigger
{
public:
    HungerForBloodTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hunger for blood") {}
    bool IsActive() override;
};

class HungerForBloodNeedsBleedTrigger : public Trigger
{
public:
    HungerForBloodNeedsBleedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "hunger for blood needs bleed") {}
    bool IsActive() override;
};

class AdrenalineRushTrigger : public BoostTrigger
{
public:
    AdrenalineRushTrigger(PlayerbotAI* botAI) : BoostTrigger(botAI, "adrenaline rush") {}

    // bool isPossible();
};

class BladeFlurryTrigger : public BoostTrigger
{
public:
    BladeFlurryTrigger(PlayerbotAI* botAI) : BoostTrigger(botAI, "blade flurry") {}
};

class RiposteAvailableTrigger : public SpellCanBeCastTrigger
{
public:
    RiposteAvailableTrigger(PlayerbotAI* botAI) : SpellCanBeCastTrigger(botAI, "riposte") {}
};

class RuptureTrigger : public DebuffTrigger
{
public:
    RuptureTrigger(PlayerbotAI* botAI) : DebuffTrigger(botAI, "rupture", 1, true) {}
};

class ExposeArmorTrigger : public DebuffTrigger
{
public:
    ExposeArmorTrigger(PlayerbotAI* botAI) : DebuffTrigger(botAI, "expose armor") {}
    virtual bool IsActive() override;
};

class KickInterruptEnemyHealerSpellTrigger : public InterruptEnemyHealerTrigger
{
public:
    KickInterruptEnemyHealerSpellTrigger(PlayerbotAI* botAI) : InterruptEnemyHealerTrigger(botAI, "kick") {}
};

class InStealthTrigger : public HasAuraTrigger
{
public:
    InStealthTrigger(PlayerbotAI* botAI) : HasAuraTrigger(botAI, "stealth") {}
};

class NoStealthTrigger : public HasNoAuraTrigger
{
public:
    NoStealthTrigger(PlayerbotAI* botAI) : HasNoAuraTrigger(botAI, "stealth") {}
};

class UnstealthTrigger : public BuffTrigger
{
public:
    UnstealthTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "stealth", 3) {}

    bool IsActive() override;
};

class StealthTrigger : public Trigger
{
public:
    StealthTrigger(PlayerbotAI* botAI) : Trigger(botAI, "stealth") {}

    bool IsActive() override;
};

class SapTrigger : public HasCcTargetTrigger
{
public:
    SapTrigger(PlayerbotAI* botAI) : HasCcTargetTrigger(botAI, "sap") {}

    bool IsPossible();
};

class SprintTrigger : public BuffTrigger
{
public:
    SprintTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "sprint", 3) {}

    bool IsPossible();
    bool IsActive() override;
};

class MainHandWeaponNoEnchantTrigger : public BuffTrigger
{
public:
    MainHandWeaponNoEnchantTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "main hand", 1) {}
    virtual bool IsActive();
};

class OffHandWeaponNoEnchantTrigger : public BuffTrigger
{
public:
    OffHandWeaponNoEnchantTrigger(PlayerbotAI* botAI) : BuffTrigger(botAI, "off hand", 1) {}
    virtual bool IsActive();
};

class TricksOfTheTradeOnMainTankTrigger : public BuffOnMainTankTrigger
{
public:
    TricksOfTheTradeOnMainTankTrigger(PlayerbotAI* botAI) : BuffOnMainTankTrigger(botAI, "tricks of the trade", true) {}
};

#endif
