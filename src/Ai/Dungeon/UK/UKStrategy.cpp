/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UKStrategy.h"
#include "UKMultipliers.h"

void WotlkDungeonUKStrategy::InitTriggers(std::vector<TriggerNode*> &triggers)
{
    // Prince Keleseth
    triggers.push_back(new TriggerNode("keleseth frost tomb",
             { NextAction("attack frost tomb", ACTION_RAID + 1) }));

    // Skarvald the Constructor & Dalronn the Controller
    triggers.push_back(new TriggerNode("dalronn priority",
             { NextAction("attack dalronn", ACTION_RAID + 1) }));

    // Ingvar the Plunderer

    // No easy way to check LoS here, the pillars do not seem to count as gameobjects.
    // Not implemented for now, unsure if this is needed as a good group can probably burst through the boss
    // and just eat the debuff.
    // triggers.push_back(new TriggerNode("ingvar dreadful roar",
    //          { NextAction("ingvar hide los", ACTION_RAID + 1) }));
    triggers.push_back(new TriggerNode("ingvar smash tank",
            { NextAction("ingvar dodge smash", ACTION_MOVE + 5) }));
    triggers.push_back(new TriggerNode("ingvar dark smash non tank",
            { NextAction("ingvar evade dark smash", ACTION_MOVE + 5) }));
    triggers.push_back(new TriggerNode("ingvar contact clearance",
            // Healer's generic close-range flee is ACTION_MOVE + 9.  Once a
            // non-tank is inside the safety ring, use the validated rear
            // point instead of allowing that generic movement to win.
            { NextAction("ingvar clear contact", ACTION_MOVE + 10) }));
    triggers.push_back(new TriggerNode("not behind ingvar",
            { NextAction("ingvar get behind", ACTION_MOVE + 1) }));
    triggers.push_back(new TriggerNode("ingvar shadow axe",
            { NextAction("ingvar avoid shadow axe", ACTION_MOVE + 6) }));
    // Standing formation for members that do not need melee contact: leave the 10 yd
    // radius of the smash cone entirely, instead of relying on the momentary rear arc.
    triggers.push_back(new TriggerNode("ingvar ranged clearance",
            { NextAction("ingvar keep range", ACTION_MOVE + 3) }));
    // Standing formation: below both smash responses and the axe evade, above the
    // ordinary rear-arc move. A single axe covers 5 yd, so members that do not need
    // melee contact hold more than that from each other before one lands.
    triggers.push_back(new TriggerNode("ingvar spread",
            { NextAction("ingvar spread", ACTION_MOVE + 2) }));

}

void WotlkDungeonUKStrategy::InitMultipliers(std::vector<Multiplier*> &multipliers)
{
    multipliers.push_back(new PrinceKelesethMultiplier(botAI));
    multipliers.push_back(new SkarvaldAndDalronnMultiplier(botAI));
    multipliers.push_back(new IngvarThePlundererMultiplier(botAI));
}
