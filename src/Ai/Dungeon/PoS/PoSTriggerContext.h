/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_POSTRIGGERCONTEXT_H
#define PLAYERBOTS_POSTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "PoSTriggers.h"

class WotlkDungeonPoSTriggerContext : public NamedObjectContext<Trigger>
{
public:
    WotlkDungeonPoSTriggerContext()
    {
        creators["ick and krick"] = &WotlkDungeonPoSTriggerContext::ick_and_krick;
        creators["tyrannus"] = &WotlkDungeonPoSTriggerContext::tyrannus;
        creators["garfrost permafrost"] = &WotlkDungeonPoSTriggerContext::garfrost_permafrost;
        creators["freezing circle"] = &WotlkDungeonPoSTriggerContext::freezing_circle;
    }

private:
    static Trigger* ick_and_krick(PlayerbotAI* ai) { return new IckAndKrickTrigger(ai); }
    static Trigger* tyrannus(PlayerbotAI* ai) { return new TyrannusTrigger(ai); }
    static Trigger* garfrost_permafrost(PlayerbotAI* ai) { return new GarfrostPermafrostTrigger(ai); }
    static Trigger* freezing_circle(PlayerbotAI* ai) { return new FreezingCircleTrigger(ai); }
};

#endif
