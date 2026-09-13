/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ANTRIGGERCONTEXT_H
#define PLAYERBOTS_ANTRIGGERCONTEXT_H

#include "ANTriggers.h"
#include "NamedObjectContext.h"

class WotlkDungeonANTriggerContext : public NamedObjectContext<Trigger>
{
    public:
        WotlkDungeonANTriggerContext()
        {
            creators["krik'thir web wrap"] = &WotlkDungeonANTriggerContext::krikthir_web_wrap;
            creators["krik'thir watchers"] = &WotlkDungeonANTriggerContext::krikthir_watchers;
            creators["anub'arak impale"] = &WotlkDungeonANTriggerContext::anubarak_impale;
            creators["anub'arak pound"] = &WotlkDungeonANTriggerContext::anubarak_pound;
            creators["anub'arak rim"] = &WotlkDungeonANTriggerContext::anubarak_rim;
            creators["anub'arak ranged too close"] = &WotlkDungeonANTriggerContext::anubarak_ranged_too_close;
            creators["anub'arak pound tank"] = &WotlkDungeonANTriggerContext::anubarak_pound_tank;
            creators["anub'arak pound healer"] = &WotlkDungeonANTriggerContext::anubarak_pound_healer;
        }
    private:
        static Trigger* krikthir_web_wrap(PlayerbotAI* ai) { return new KrikthirWebWrapTrigger(ai); }
        static Trigger* krikthir_watchers(PlayerbotAI* ai) { return new KrikthirWatchersTrigger(ai); }
        static Trigger* anubarak_impale(PlayerbotAI* ai) { return new AnubarakImpaleTrigger(ai); }
        static Trigger* anubarak_pound(PlayerbotAI* ai) { return new AnubarakPoundTrigger(ai); }
        static Trigger* anubarak_rim(PlayerbotAI* ai) { return new AnubarakRimTrigger(ai); }
        static Trigger* anubarak_ranged_too_close(PlayerbotAI* ai) { return new AnubarakRangedTooCloseTrigger(ai); }
        static Trigger* anubarak_pound_tank(PlayerbotAI* ai) { return new AnubarakPoundTankTrigger(ai); }
        static Trigger* anubarak_pound_healer(PlayerbotAI* ai) { return new AnubarakPoundHealerTrigger(ai); }
};

#endif
