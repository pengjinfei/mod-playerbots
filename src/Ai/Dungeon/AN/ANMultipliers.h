/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ANMULTIPLIERS_H
#define PLAYERBOTS_ANMULTIPLIERS_H

#include "Multiplier.h"

class KrikthirMultiplier : public Multiplier
{
    public:
        KrikthirMultiplier(PlayerbotAI* ai) : Multiplier(ai, "krik'thir the gatewatcher") {}

    public:
        float GetValue(Action* action) override;
};

// Anub'arak: a fire mage at ilvl 187 (~13k mana) is out of mana by 90-120 s because it spreads Living Bomb
// over every add (19 casts / 255 s, ~720 each) and channels Blizzard (~2,400 each) / Flamestrike (~980)
// on the 7.5x-damage adds. The adds are tank-held and low on health; single-target casts are cheaper per
// kill. Zero those three actions while Anub'arak is the encounter (run 461 mana curves, 2026-09-13).
class AnubarakMageManaMultiplier : public Multiplier
{
    public:
        AnubarakMageManaMultiplier(PlayerbotAI* ai) : Multiplier(ai, "anub'arak mage mana") {}

    public:
        float GetValue(Action* action) override;
};

#endif
