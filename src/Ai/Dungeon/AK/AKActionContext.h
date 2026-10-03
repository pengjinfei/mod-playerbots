/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AKACTIONCONTEXT_H
#define PLAYERBOTS_AKACTIONCONTEXT_H

#include "AKActions.h"
#include "Action.h"
#include "NamedObjectContext.h"

class WotlkDungeonOKActionContext : public NamedObjectContext<Action>
{
    public:
        WotlkDungeonOKActionContext() {
            creators["attack nadox guardian"] = &WotlkDungeonOKActionContext::attack_nadox_guardian;
            creators["attack jedoga volunteer"] = &WotlkDungeonOKActionContext::attack_jedoga_volunteer;
            creators["attack jedoga worshipper"] = &WotlkDungeonOKActionContext::attack_jedoga_worshipper;
            creators["attack taldaram embracing"] = &WotlkDungeonOKActionContext::attack_taldaram_embracing;
            creators["avoid shadow crash"] = &WotlkDungeonOKActionContext::avoid_shadow_crash;
            creators["avoid flame sphere"] = &WotlkDungeonOKActionContext::avoid_flame_sphere;
        }
    private:
        static Action* attack_nadox_guardian(PlayerbotAI* ai) { return new AttackNadoxGuardianAction(ai); }
        static Action* attack_jedoga_volunteer(PlayerbotAI* ai) { return new AttackJedogaVolunteerAction(ai); }
        static Action* attack_jedoga_worshipper(PlayerbotAI* ai) { return new AttackJedogaWorshipperAction(ai); }
        static Action* attack_taldaram_embracing(PlayerbotAI* ai) { return new AttackTaldaramEmbracingAction(ai); }
        static Action* avoid_shadow_crash(PlayerbotAI* ai) { return new AvoidShadowCrashAction(ai); }
        static Action* avoid_flame_sphere(PlayerbotAI* ai) { return new AvoidFlameSphereAction(ai); }
};

#endif
