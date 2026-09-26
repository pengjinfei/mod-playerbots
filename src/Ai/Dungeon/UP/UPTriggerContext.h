/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_UPTRIGGERCONTEXT_H
#define PLAYERBOTS_UPTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "UPTriggers.h"

class WotlkDungeonUPTriggerContext : public NamedObjectContext<Trigger>
{
    public:
        WotlkDungeonUPTriggerContext()
        {
            creators["freezing cloud"] = &WotlkDungeonUPTriggerContext::freezing_cloud;
            creators["skadi whirlwind"] = &WotlkDungeonUPTriggerContext::whirlwind;
            creators["ymiron bane"] = &WotlkDungeonUPTriggerContext::bane;
            creators["skadi harpoon pickup"] = &WotlkDungeonUPTriggerContext::skadi_harpoon_pickup;
            creators["skadi harpoon launch"] = &WotlkDungeonUPTriggerContext::skadi_harpoon_launch;
        }
    private:
        static Trigger* skadi_harpoon_pickup(PlayerbotAI* ai) { return new SkadiHarpoonPickupTrigger(ai); }
        static Trigger* skadi_harpoon_launch(PlayerbotAI* ai) { return new SkadiHarpoonLaunchTrigger(ai); }
        static Trigger* freezing_cloud(PlayerbotAI* ai) { return new SkadiFreezingCloudTrigger(ai); }
        static Trigger* whirlwind(PlayerbotAI* ai) { return new SkadiWhirlwindTrigger(ai); }
        static Trigger* bane(PlayerbotAI* ai) { return new YmironBaneTrigger(ai); }
};

#endif
