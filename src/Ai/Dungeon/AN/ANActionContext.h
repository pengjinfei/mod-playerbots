/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ANACTIONCONTEXT_H
#define PLAYERBOTS_ANACTIONCONTEXT_H

#include "ANActions.h"
#include "Action.h"
#include "NamedObjectContext.h"

class WotlkDungeonANActionContext : public NamedObjectContext<Action>
{
    public:
        WotlkDungeonANActionContext() {
            creators["attack web wrap"] = &WotlkDungeonANActionContext::attack_web_wrap;
            creators["krik'thir priority"] = &WotlkDungeonANActionContext::krikthir_priority;
            creators["hadronox tank leave acid"] = &WotlkDungeonANActionContext::hadronox_tank_leave_acid;
            creators["dodge pound"] = &WotlkDungeonANActionContext::dodge_pound;
            creators["dodge impale"] = &WotlkDungeonANActionContext::dodge_impale;
            creators["anub'arak rim guard"] = &WotlkDungeonANActionContext::anubarak_rim_guard;
            creators["anub'arak keep range"] = &WotlkDungeonANActionContext::anubarak_keep_range;
            creators["anub'arak pound shield tank"] = &WotlkDungeonANActionContext::anubarak_pound_shield_tank;
            creators["anub'arak pound heal tank"] = &WotlkDungeonANActionContext::anubarak_pound_heal_tank;
            creators["anub'arak focus venomancer"] = &WotlkDungeonANActionContext::anubarak_focus_venomancer;
            creators["anub'arak offheal"] = &WotlkDungeonANActionContext::anubarak_offheal;
            creators["anub'arak melee behind"] = &WotlkDungeonANActionContext::anubarak_melee_behind;
        }
    private:
        static Action* attack_web_wrap(PlayerbotAI* ai) { return new AttackWebWrapAction(ai); }
        static Action* krikthir_priority(PlayerbotAI* ai) { return new WatchersTargetAction(ai); }
        static Action* hadronox_tank_leave_acid(PlayerbotAI* ai) { return new HadronoxTankLeaveAcidAction(ai); }
        static Action* dodge_pound(PlayerbotAI* ai) { return new AnubarakDodgePoundAction(ai); }
        static Action* dodge_impale(PlayerbotAI* ai) { return new AnubarakDodgeImpaleAction(ai); }
        static Action* anubarak_rim_guard(PlayerbotAI* ai) { return new AnubarakRimGuardAction(ai); }
        static Action* anubarak_keep_range(PlayerbotAI* ai) { return new AnubarakKeepRangeAction(ai); }
        static Action* anubarak_pound_shield_tank(PlayerbotAI* ai) { return new AnubarakPoundShieldTankAction(ai); }
        static Action* anubarak_pound_heal_tank(PlayerbotAI* ai) { return new AnubarakPoundHealTankAction(ai); }
        static Action* anubarak_focus_venomancer(PlayerbotAI* ai) { return new AnubarakFocusVenomancerAction(ai); }
        static Action* anubarak_offheal(PlayerbotAI* ai) { return new AnubarakOffhealAction(ai); }
        static Action* anubarak_melee_behind(PlayerbotAI* ai) { return new AnubarakMeleeBehindAction(ai); }
};

#endif
