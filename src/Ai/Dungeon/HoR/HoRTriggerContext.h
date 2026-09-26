/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORTRIGGERCONTEXT_H
#define PLAYERBOTS_HORTRIGGERCONTEXT_H

#include "HoRTriggers.h"
#include "NamedObjectContext.h"

class WotlkDungeonHoRTriggerContext : public NamedObjectContext<Trigger>
{
public:
    WotlkDungeonHoRTriggerContext()
    {
        creators["hor wave boss boost"] = &WotlkDungeonHoRTriggerContext::hor_wave_boss_boost;
        creators["hor escape keep up"] = &WotlkDungeonHoRTriggerContext::hor_escape_keep_up;
    }

private:
    static Trigger* hor_escape_keep_up(PlayerbotAI* ai) { return new HoREscapeKeepUpTrigger(ai); }
    static Trigger* hor_wave_boss_boost(PlayerbotAI* ai) { return new HoRWaveBossBoostTrigger(ai); }
};

#endif
