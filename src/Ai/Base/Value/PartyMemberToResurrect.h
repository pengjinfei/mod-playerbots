/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_PARTYMEMBERTORESURRECT_H
#define PLAYERBOTS_PARTYMEMBERTORESURRECT_H

#include "PartyMemberValue.h"

class PlayerbotAI;
class Unit;

class PartyMemberToResurrect : public PartyMemberValue
{
public:
    PartyMemberToResurrect(PlayerbotAI* botAI, std::string const name = "party member to resurrect")
        : PartyMemberValue(botAI, name)
    {
    }

protected:
    Unit* Calculate() override;
    // A corpse out of sight is still one to resurrect: its reach action walks into sight first. With the sight
    // filter a rogue dead on Taldaram's lowered platform, 6 yd from the priest but behind its rim, was never chosen
    // and the run waited for it until it stalled (Ahn'kahet, run 2003).
    bool Check(Unit* player) override;
};

#endif
