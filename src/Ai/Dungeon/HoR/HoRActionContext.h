/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HORACTIONCONTEXT_H
#define PLAYERBOTS_HORACTIONCONTEXT_H

#include "HoRActions.h"
#include "NamedObjectContext.h"

class WotlkDungeonHoRActionContext : public NamedObjectContext<Action>
{
public:
    WotlkDungeonHoRActionContext()
    {
        creators["hor escape keep up"] = &WotlkDungeonHoRActionContext::hor_escape_keep_up;
    }

private:
    static Action* hor_escape_keep_up(PlayerbotAI* ai) { return new HoREscapeKeepUpAction(ai); }
};

#endif
