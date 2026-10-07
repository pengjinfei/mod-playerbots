/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoSMultipliers.h"

#include <set>
#include <string>
#include "Action.h"
#include "ChooseTargetActions.h"
#include "GenericSpellActions.h"
#include "HoSActions.h"
#include "HoSTriggers.h"
#include "MovementActions.h"

float KrystallusMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "krystallus");
    if (!boss) { return 1.0f; }

    // No elemental beside the group: a Greater Fire Elemental is stoned by Ground Slam like a player and its Shatter
    // hit every member for 8-12k on top of theirs - three died to one Shatter that they would have lived through
    // without it (Halls of Stone full run 2347). Mirror images alike: each Shattered for 25k on every member (2351,
    // 2352). No summoned helpers at all.
    static std::set<std::string> const summons = {"fire elemental totem", "earth elemental totem", "mirror image",
                                                  "shadowfiend", "feral spirit", "army of the dead", "summon gargoyle"};
    if (summons.count(action->getName()))
        return 0.0f;

    // Check both of these... the spell is applied first, debuff later.
    // Neither is active for the full duration so we need to trigger off both
    if (bot->HasAura(SPELL_GROUND_SLAM) || bot->HasAura(DEBUFF_GROUND_SLAM))
    {
        if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<ShatterSpreadAction*>(action))
        {
            return 0.0f;
        }
    }
    return 1.0f;
}

float SjonnirMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "sjonnir the ironshaper");
    if (!boss) { return 1.0f; }

    if (boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(SPELL_LIGHTNING_RING))
        {
            // Problematic since there's a lot of movement on this boss, will prevent players from positioning
            // well to deal with adds etc. during the channel period. Takes a bit of work to improve this though
            if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<AvoidLightningRingAction*>(action))
            {
                return 0.0f;
            }
        }
    return 1.0f;
}
