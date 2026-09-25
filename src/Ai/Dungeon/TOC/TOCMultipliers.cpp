/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "TOCMultipliers.h"
#include "Action.h"
#include "ChooseTargetActions.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "TOCActions.h"
#include "TOCTriggers.h"

//float tocMultiplier::GetValue(Action* action) { return 1.0f; }

float PaletressShieldMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "argent confessor paletress");
    if (!boss || !boss->HasAura(SPELL_PALETRESS_REFLECTIVE_SHIELD))
        return 1.0f;

    bool memoryAlive = false;
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit != boss && unit->IsAlive() && unit->IsInCombat())
        {
            memoryAlive = true;
            break;
        }
    }
    if (!memoryAlive)
        return 1.0f;

    if (dynamic_cast<ToCPaletressShieldAction*>(action))
        return 1.0f;

    Unit* current = AI_VALUE(Unit*, "current target");
    if (dynamic_cast<AttackAction*>(action) && (action->GetTarget() == boss || current == boss))
        return 0.0f;

    if (CastSpellAction* spell = dynamic_cast<CastSpellAction*>(action))
        if (spell->GetTargetName() == "current target" && current == boss)
            return 0.0f;

    return 1.0f;
}
