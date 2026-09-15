/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_UKTRIGGERCONTEXT_H
#define PLAYERBOTS_UKTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "UKTriggers.h"

class WotlkDungeonUKTriggerContext : public NamedObjectContext<Trigger>
{
    public:
        WotlkDungeonUKTriggerContext()
        {
            creators["keleseth frost tomb"] = &WotlkDungeonUKTriggerContext::keleseth_frost_tomb;
            creators["dalronn priority"] = &WotlkDungeonUKTriggerContext::dalronn_priority_target;
            creators["ingvar dreadful roar"] = &WotlkDungeonUKTriggerContext::ingvar_dreadful_roar;
            creators["ingvar smash tank"] = &WotlkDungeonUKTriggerContext::ingvar_smash_tank;
            creators["ingvar dark smash non tank"] = &WotlkDungeonUKTriggerContext::ingvar_dark_smash_non_tank;
            creators["ingvar contact clearance"] = &WotlkDungeonUKTriggerContext::ingvar_contact_clearance;
            creators["not behind ingvar"] = &WotlkDungeonUKTriggerContext::not_behind_ingvar;
            creators["ingvar shadow axe"] = &WotlkDungeonUKTriggerContext::ingvar_shadow_axe;
            creators["ingvar spread"] = &WotlkDungeonUKTriggerContext::ingvar_spread;
            creators["ingvar ranged clearance"] = &WotlkDungeonUKTriggerContext::ingvar_ranged_clearance;
            creators["ingvar los lost"] = &WotlkDungeonUKTriggerContext::ingvar_los_lost;
        }
    private:
        static Trigger* keleseth_frost_tomb(PlayerbotAI* ai) { return new KelesethFrostTombTrigger(ai); }
        static Trigger* dalronn_priority_target(PlayerbotAI* ai) { return new DalronnDpsTrigger(ai); }
        static Trigger* ingvar_dreadful_roar(PlayerbotAI* ai) { return new IngvarDreadfulRoarTrigger(ai); }
        static Trigger* ingvar_smash_tank(PlayerbotAI* ai) { return new IngvarSmashTankTrigger(ai); }
        static Trigger* ingvar_dark_smash_non_tank(PlayerbotAI* ai) { return new IngvarDarkSmashNonTankTrigger(ai); }
        static Trigger* ingvar_contact_clearance(PlayerbotAI* ai) { return new IngvarContactClearanceTrigger(ai); }
        static Trigger* not_behind_ingvar(PlayerbotAI* ai) { return new NotBehindIngvarTrigger(ai); }
        static Trigger* ingvar_shadow_axe(PlayerbotAI* ai) { return new IngvarShadowAxeTrigger(ai); }
        static Trigger* ingvar_spread(PlayerbotAI* ai) { return new IngvarSpreadTrigger(ai); }
        static Trigger* ingvar_ranged_clearance(PlayerbotAI* ai) { return new IngvarRangedClearanceTrigger(ai); }
        static Trigger* ingvar_los_lost(PlayerbotAI* ai) { return new IngvarLosLostTrigger(ai); }
};

#endif
