/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NEXACTIONCONTEXT_H
#define PLAYERBOTS_NEXACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "NexActions.h"

class WotlkDungeonNexActionContext : public NamedObjectContext<Action>
{
    public:
        WotlkDungeonNexActionContext() {
            creators["move from whirlwind"] = &WotlkDungeonNexActionContext::move_from_whirlwind;
            creators["firebomb spread"] = &WotlkDungeonNexActionContext::firebomb_spread;
            creators["telestra split target"] = &WotlkDungeonNexActionContext::telestra_split_target;
            creators["chaotic rift target"] = &WotlkDungeonNexActionContext::chaotic_rift_target;
            creators["trash cc mark"] = &WotlkDungeonNexActionContext::trash_cc_mark;
            creators["trash cc polymorph"] = &WotlkDungeonNexActionContext::trash_cc_polymorph;
            creators["trash cc hex"] = &WotlkDungeonNexActionContext::trash_cc_hex;
            creators["trash cc sap"] = &WotlkDungeonNexActionContext::trash_cc_sap;
            creators["dodge spikes"] = &WotlkDungeonNexActionContext::dodge_spikes;
            creators["intense cold jump"] = &WotlkDungeonNexActionContext::intense_cold_jump;
        }
    private:
        static Action* move_from_whirlwind(PlayerbotAI* ai) { return new MoveFromWhirlwindAction(ai); }
        static Action* firebomb_spread(PlayerbotAI* ai) { return new FirebombSpreadAction(ai); }
        static Action* telestra_split_target(PlayerbotAI* ai) { return new TelestraSplitTargetAction(ai); }
        static Action* chaotic_rift_target(PlayerbotAI* ai) { return new ChaoticRiftTargetAction(ai); }
        static Action* trash_cc_mark(PlayerbotAI* ai) { return new TrashCcMarkAction(ai); }
        static Action* trash_cc_polymorph(PlayerbotAI* ai) { return new TrashCcPolymorphAction(ai); }
        static Action* trash_cc_hex(PlayerbotAI* ai) { return new TrashCcHexAction(ai); }
        static Action* trash_cc_sap(PlayerbotAI* ai) { return new TrashCcSapAction(ai); }
        static Action* dodge_spikes(PlayerbotAI* ai) { return new DodgeSpikesAction(ai); }
        static Action* intense_cold_jump(PlayerbotAI* ai) { return new IntenseColdJumpAction(ai); }
};

#endif
