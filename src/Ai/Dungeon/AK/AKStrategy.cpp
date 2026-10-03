/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AKStrategy.h"
#include "AKMultipliers.h"

void WotlkDungeonOKStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Elder Nadox
    triggers.push_back(new TriggerNode("nadox guardian",
        { NextAction("attack nadox guardian", ACTION_RAID + 5) }));

    // Prince Taldaram
    // Flame spheres spawn on him and move out 25 yd in three directions; on heroic three of them killed four of five
    // standing in them (runs 1970/1971). Step out of their way.
    triggers.push_back(new TriggerNode("taldaram flame sphere",
        { NextAction("avoid flame sphere", ACTION_MOVE + 5) }));
    // His Embrace of the Vampyr breaks after enough damage: everyone back on him the moment he starts it.
    triggers.push_back(new TriggerNode("taldaram embrace",
        { NextAction("attack taldaram embracing", ACTION_RAID + 4) }));

    // Jedoga Shadowseeker
    triggers.push_back(new TriggerNode("jedoga volunteer",
        { NextAction("attack jedoga volunteer", ACTION_RAID + 5) }));
    // Her worshippers join the fight casting Fireball round the room: the casters first.
    triggers.push_back(new TriggerNode("jedoga worshipper",
        { NextAction("attack jedoga worshipper", ACTION_RAID + 3) }));

    // Herald Volazj
    // Trash mobs before him have a big telegraphed shadow crash spell,
    // this can be avoided and is intended to be dodged
    triggers.push_back(new TriggerNode("shadow crash",
        { NextAction("avoid shadow crash", ACTION_MOVE + 5) }));
    // Volazj is not implemented properly in AC, insanity phase does nothing.

    // Amanitar (Heroic Only)
    // TODO: once I get to heroics
}

void WotlkDungeonOKStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new ElderNadoxMultiplier(botAI));
    multipliers.push_back(new JedogaShadowseekerMultiplier(botAI));
    multipliers.push_back(new JedogaKneelingWorshippersMultiplier(botAI));
    multipliers.push_back(new ForgottenOneMultiplier(botAI));
}
