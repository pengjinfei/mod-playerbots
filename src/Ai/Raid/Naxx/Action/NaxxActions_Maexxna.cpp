/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"
#include "NaxxSpellIds.h"
#include "Playerbots.h"

bool MaexxnaCureNecroticPoisonAction::Execute(Event /*event*/)
{
    Unit* mt = AI_VALUE(Unit*, "main tank");
    if (!mt)
        return false;

    switch (bot->getClass())
    {
        case CLASS_PALADIN:
            return botAI->CastSpell("cleanse", mt);
        case CLASS_SHAMAN:
            return botAI->CastSpell("cure toxins", mt);
        case CLASS_DRUID:
            return botAI->CastSpell("abolish poison", mt) || botAI->CastSpell("cure poison", mt);
        default:
            return false;
    }
}

bool MaexxnaAttackWebWrapAction::Execute(Event /*event*/)
{
    Creature* wrap = bot->FindNearestCreature(NaxxSpellIds::NpcWebWrap, 50.0f, true);
    if (!wrap || AI_VALUE(Unit*, "current target") == wrap)
        return false;

    return Attack(wrap);
}
