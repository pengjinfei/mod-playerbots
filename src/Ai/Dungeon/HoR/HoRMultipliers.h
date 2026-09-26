/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORMULTIPLIERS_H
#define PLAYERBOTS_HORMULTIPLIERS_H

#include "Multiplier.h"

// Falric's and Marwyn's waves: hold Heroism/Bloodlust for the boss instead of the first wave of spirits.
class HoRHoldHeroismForBossMultiplier : public Multiplier
{
public:
    HoRHoldHeroismForBossMultiplier(PlayerbotAI* ai) : Multiplier(ai, "hor hold heroism for boss") {}

    float GetValue(Action* action) override;
};

#endif
