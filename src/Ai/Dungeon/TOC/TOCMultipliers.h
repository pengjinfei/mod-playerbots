/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_TOCMULTIPLIERS_H
#define PLAYERBOTS_TOCMULTIPLIERS_H

#include "Multiplier.h"

// While Paletress's Reflective Shield is up and her Memory is alive, nothing may attack or cast at her.
class PaletressShieldMultiplier : public Multiplier
{
public:
    PaletressShieldMultiplier(PlayerbotAI* ai) : Multiplier(ai, "paletress shield") {}
    float GetValue(Action* action) override;
};

/* class tocMultiplier : public Multiplier
{
    public:
    tocMultiplier(PlayerbotAI* ai) : Multiplier(ai, "toc") {}

    public:
        float GetValue(Action* action) override;
};
*/
#endif
