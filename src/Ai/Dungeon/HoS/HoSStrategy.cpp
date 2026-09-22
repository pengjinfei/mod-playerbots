/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoSStrategy.h"
#include "HoSMultipliers.h"

void WotlkDungeonHoSStrategy::InitTriggers(std::vector<TriggerNode*> &triggers)
{
    // Maiden of Grief
    // TODO: Jump into damage during shock of sorrow?

    // Krystallus
    // TODO: I think bots need to dismiss pets on this, or they nuke players they are standing close to
    triggers.push_back(new TriggerNode("ground slam",
        { NextAction("shatter spread", ACTION_RAID + 5) }));

    // Tribunal of Ages
    // Reconnect a tank with the healer-side fight only when LOS filtering has left
    // it with no attacker or current target; the action paths but does not attack.
    triggers.push_back(new TriggerNode("tribunal los reacquire",
        { NextAction("tribunal los reacquire", ACTION_RAID + 5) }));

    // Searing Gaze creates a short-lived ground trigger at a selected player's
    // position. Leave it rather than changing targets or tank threat.
    triggers.push_back(new TriggerNode("tribunal searing gaze",
        { NextAction("tribunal flee searing gaze", ACTION_EMERGENCY + 10) }));

    // Sjonnir The Ironshaper
    // Possibly tank in place in the middle of the room, assign a dps to adds?
    triggers.push_back(new TriggerNode("lightning ring",
        { NextAction("avoid lightning ring", ACTION_RAID + 5) }));
}

void WotlkDungeonHoSStrategy::InitMultipliers(std::vector<Multiplier*> &multipliers)
{
    multipliers.push_back(new KrystallusMultiplier(botAI));
    multipliers.push_back(new SjonnirMultiplier(botAI));
}
