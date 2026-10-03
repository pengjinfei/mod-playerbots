/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AKMultipliers.h"
#include "AKActions.h"
#include "AKTriggers.h"
#include "Action.h"
#include "ChooseTargetActions.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "ReachTargetActions.h"

float ElderNadoxMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "elder nadox");
    if (!boss) { return 1.0f; }

    Unit* guardian = AI_VALUE2(Unit*, "find target", "ahn'kahar guardian");
    if (guardian)
    {
        if (dynamic_cast<DpsAssistAction*>(action))
        {
            return 0.0f;
        }
    }
    return 1.0f;
}

// Her ten worshippers kneel in two groups of five, 55-70 yd off her, and stay there unless someone comes near. Chasing a
// fleeing Twilight Initiate the tank went to 15 yd of the south group, all five came with Jedoga and the group wiped
// (run 2044). As players do: melee let a target that runs to them go, the casters finish it from range.
float JedogaKneelingWorshippersMultiplier::GetValue(Action* action)
{
    if (!dynamic_cast<ReachTargetAction*>(action) || botAI->IsRanged(bot))
        return 1.0f;
    if (!AI_VALUE2(Unit*, "find target", "jedoga shadowseeker"))
        return 1.0f;
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || target->GetEntry() == NPC_TWILIGHT_WORSHIPPER_OK)
        return 1.0f;
    std::list<Creature*> worshippers;
    target->GetCreatureListWithEntryInGrid(worshippers, NPC_TWILIGHT_WORSHIPPER_OK, JEDOGA_KNEELING_CLEARANCE);
    for (Creature* worshipper : worshippers)
        if (worshipper->IsAlive() && !worshipper->IsInCombat())
            return 0.0f;
    return 1.0f;
}

float JedogaShadowseekerMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "jedoga shadowseeker");
    if (!boss) { return 1.0f; }

    Unit* volunteer = nullptr;
    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_TWILIGHT_VOLUNTEER)
        {
            volunteer = unit;
            break;
        }
    }

    if (volunteer)
    {
        if (dynamic_cast<DpsAssistAction*>(action))
        {
            return 0.0f;
        }
    }
    return 1.0f;
}

float ForgottenOneMultiplier::GetValue(Action* action)
{
    Unit* unit = AI_VALUE2(Unit*, "find target", "forgotten one");
    if (!unit) { return 1.0f; }

    if (bot->isMoving())
    {
        if (dynamic_cast<MovementAction*>(action))
        {
            return 0.0f;
        }
    }
    return 1.0f;
}
