/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HOSTRIGGERCONTEXT_H
#define PLAYERBOTS_HOSTRIGGERCONTEXT_H

#include "HoSTriggers.h"
#include "NamedObjectContext.h"

class WotlkDungeonHoSTriggerContext : public NamedObjectContext<Trigger>
{
    public:
        WotlkDungeonHoSTriggerContext()
        {
            creators["ground slam"] = &WotlkDungeonHoSTriggerContext::ground_slam;
            creators["lightning ring"] = &WotlkDungeonHoSTriggerContext::lightning_ring;
            creators["tribunal los reacquire"] = &WotlkDungeonHoSTriggerContext::tribunal_los_reacquire;
            creators["tribunal searing gaze"] = &WotlkDungeonHoSTriggerContext::tribunal_searing_gaze;
        }
    private:
        static Trigger* ground_slam(PlayerbotAI* ai) { return new KrystallusGroundSlamTrigger(ai); }
        static Trigger* lightning_ring(PlayerbotAI* ai) { return new SjonnirLightningRingTrigger(ai); }
        static Trigger* tribunal_los_reacquire(PlayerbotAI* ai) { return new TribunalLosReacquireTrigger(ai); }
        static Trigger* tribunal_searing_gaze(PlayerbotAI* ai) { return new TribunalSearingGazeTrigger(ai); }
};

#endif
