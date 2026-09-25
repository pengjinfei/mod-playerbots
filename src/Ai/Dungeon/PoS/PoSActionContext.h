/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_POSACTIONCONTEXT_H
#define PLAYERBOTS_POSACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "PoSActions.h"

class WotlkDungeonPoSActionContext : public NamedObjectContext<Action>
{
    public:
        WotlkDungeonPoSActionContext()
        {
            creators["ick and krick"] = &WotlkDungeonPoSActionContext::ick_and_krick;
            creators["tyrannus"] = &WotlkDungeonPoSActionContext::tyrannus;
            creators["garfrost hide behind rock"] = &WotlkDungeonPoSActionContext::garfrost_hide_behind_rock;
        }
    private:
        static Action* ick_and_krick(PlayerbotAI* ai) { return new IckAndKrickAction(ai); }
        static Action* tyrannus(PlayerbotAI* ai) { return new TyrannusAction(ai); }
        static Action* garfrost_hide_behind_rock(PlayerbotAI* ai) { return new GarfrostHideBehindRockAction(ai); }
};

#endif
