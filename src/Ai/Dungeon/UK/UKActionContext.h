/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_UKACTIONCONTEXT_H
#define PLAYERBOTS_UKACTIONCONTEXT_H

#include "Action.h"
#include "MovementActions.h"
#include "NamedObjectContext.h"
#include "TellLosAction.h"
#include "UKActions.h"

class WotlkDungeonUKActionContext : public NamedObjectContext<Action>
{
    public:
        WotlkDungeonUKActionContext() {
            creators["attack frost tomb"] = &WotlkDungeonUKActionContext::attack_frost_tomb;
            creators["attack dalronn"] = &WotlkDungeonUKActionContext::attack_dalronn;
            creators["ingvar get behind"] = &WotlkDungeonUKActionContext::ingvar_get_behind;
            creators["ingvar evade dark smash"] = &WotlkDungeonUKActionContext::ingvar_evade_dark_smash;
            creators["ingvar clear contact"] = &WotlkDungeonUKActionContext::ingvar_clear_contact;
            // creators["ingvar hide los"] = &WotlkDungeonUKActionContext::ingvar_hide_los;
            creators["ingvar dodge smash"] = &WotlkDungeonUKActionContext::ingvar_dodge_smash;
            creators["ingvar avoid shadow axe"] = &WotlkDungeonUKActionContext::ingvar_avoid_shadow_axe;
            creators["ingvar spread"] = &WotlkDungeonUKActionContext::ingvar_spread;
            creators["ingvar keep range"] = &WotlkDungeonUKActionContext::ingvar_keep_range;
            creators["ingvar regain los"] = &WotlkDungeonUKActionContext::ingvar_regain_los;
        }
    private:
        static Action* attack_frost_tomb(PlayerbotAI* ai) { return new AttackFrostTombAction(ai); }
        static Action* attack_dalronn(PlayerbotAI* ai) { return new AttackDalronnAction(ai); }
        static Action* ingvar_get_behind(PlayerbotAI* ai) { return new IngvarGetBehindAction(ai); }
        static Action* ingvar_evade_dark_smash(PlayerbotAI* ai) { return new IngvarEvadeDarkSmashAction(ai); }
        static Action* ingvar_clear_contact(PlayerbotAI* ai) { return new IngvarClearContactAction(ai); }
        // static Action* ingvar_hide_los(PlayerbotAI* ai) { return new TellLosAction(ai); }
        static Action* ingvar_dodge_smash(PlayerbotAI* ai) { return new IngvarDodgeSmashAction(ai); }
        static Action* ingvar_avoid_shadow_axe(PlayerbotAI* ai) { return new IngvarAvoidShadowAxeAction(ai); }
        static Action* ingvar_spread(PlayerbotAI* ai) { return new IngvarSpreadAction(ai); }
        static Action* ingvar_keep_range(PlayerbotAI* ai) { return new IngvarKeepRangeAction(ai); }
        static Action* ingvar_regain_los(PlayerbotAI* ai) { return new IngvarRegainLosAction(ai); }
};

#endif
