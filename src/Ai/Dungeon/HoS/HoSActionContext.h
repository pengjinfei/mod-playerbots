/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HOSACTIONCONTEXT_H
#define PLAYERBOTS_HOSACTIONCONTEXT_H

#include "Action.h"
#include "HoSActions.h"
#include "NamedObjectContext.h"

class WotlkDungeonHoSActionContext : public NamedObjectContext<Action>
{
    public:
        WotlkDungeonHoSActionContext() {
            creators["shatter spread"] = &WotlkDungeonHoSActionContext::shatter_spread;
            creators["avoid lightning ring"] = &WotlkDungeonHoSActionContext::avoid_lightning_ring;
            creators["tribunal los reacquire"] = &WotlkDungeonHoSActionContext::tribunal_los_reacquire;
            creators["tribunal ranged los regain"] = &WotlkDungeonHoSActionContext::tribunal_ranged_los_regain;
            creators["tribunal flee searing gaze"] = &WotlkDungeonHoSActionContext::tribunal_flee_searing_gaze;
        }
    private:
        static Action* shatter_spread(PlayerbotAI* ai) { return new ShatterSpreadAction(ai); }
        static Action* avoid_lightning_ring(PlayerbotAI* ai) { return new AvoidLightningRingAction(ai); }
        static Action* tribunal_los_reacquire(PlayerbotAI* ai) { return new TribunalLosReacquireAction(ai); }
        static Action* tribunal_ranged_los_regain(PlayerbotAI* ai) { return new TribunalRangedLosRegainAction(ai); }
        static Action* tribunal_flee_searing_gaze(PlayerbotAI* ai) { return new TribunalFleeSearingGazeAction(ai); }
};

#endif
