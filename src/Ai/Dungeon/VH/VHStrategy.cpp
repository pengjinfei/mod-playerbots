/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "VHStrategy.h"
#include "ChooseTargetActions.h"
#include "MovementActions.h"
#include "VHMultipliers.h"

void WotlkDungeonVHStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Portals: the keeper first
    triggers.push_back(new TriggerNode("portal keeper",
        { NextAction("attack portal keeper", ACTION_RAID + 1) }));

    // Erekem
    // This boss has many purgable buffs, purging/dispels could be merged into generic strats though
    triggers.push_back(new TriggerNode("erekem target",
        { NextAction("attack erekem", ACTION_RAID + 1) }));

    // Moragg
    // TODO: This guy has Optic Link which may require moving, add if needed

    // Ichoron
    triggers.push_back(new TriggerNode("ichoron target",
        { NextAction("attack ichor globule", ACTION_RAID + 1) }));

    // Xevozz
    // Not kited: three ways of walking him off his Ethereal Spheres (step clear of them, step off one 6 yd from him,
    // step to the point farthest from all of them) each lost the isolated fight (1/5, 0/1, 0/4: runs 2290-2296,
    // 2312-2315) against 4/5 fought where he stands (2298-2302). Open: in the full run the spheres reach him before
    // his first Arcane Barrage Volley (18-21k against 0-9k in isolation) and the group dies (runs 2289, 2305, 2310).

    // Lavanthor
    // Tank & spank

    // Zuramat the Obliterator
    triggers.push_back(new TriggerNode("shroud of darkness",
        { NextAction("drop target", ACTION_HIGH + 5) }));
    triggers.push_back(new TriggerNode("void shift",
        { NextAction("attack void sentry", ACTION_RAID + 1) }));

    // Cyanigosa
    triggers.push_back(new TriggerNode("cyanigosa positioning",
        { NextAction("rear flank", ACTION_MOVE + 5) }));
}

void WotlkDungeonVHStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new ErekemMultiplier(botAI));
    multipliers.push_back(new IchoronMultiplier(botAI));
    multipliers.push_back(new ZuramatMultiplier(botAI));
}
