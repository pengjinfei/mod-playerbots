/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "PoSTriggers.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

bool IckAndKrickTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "Ick");
    if (!boss)
        return false;

    return true;
}

bool GarfrostPermafrostTrigger::IsActive()
{
    // Permafrost skips any target with a Saronite Rock between it and Garfrost. A healer behind a rock loses line of
    // sight to the tank, and melee damage dealers stepping out cost more damage than the stacks they save (heroic
    // run1288-1291 with melee hiding: 0/4, boss at 39-69%, against 1/3 ranged-only), so ranged and the tank use it.
    // The tank goes later and takes Garfrost with him: the boss has to walk around the rock, and each stack lasts
    // only 3 s without a refresh (70336), so a few seconds out of his line of sight clear them all.
    bool const tank = botAI->IsTank(bot);
    if (!tank && (botAI->IsHeal(bot) || !botAI->IsRanged(bot)))
        return false;

    // Hide once the stacks start to hurt; behind the rock they are no longer refreshed, the aura runs out
    // and the trigger releases the bot back to its rotation.
    uint8 const hideAtStacks = tank ? 8 : 6;
    Aura* permafrost = bot->GetAura(SPELL_PERMAFROST_AURA_HC);
    if (!permafrost)
        permafrost = bot->GetAura(SPELL_PERMAFROST_AURA);
    if (!permafrost || permafrost->GetStackAmount() < hideAtStacks)
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "forgemaster garfrost");
    if (!boss || !boss->IsInCombat())
        return false;

    return bot->FindNearestGameObject(GO_SARONITE_ROCK, 80.0f) != nullptr;
}

bool TyrannusTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "scourgelord tyrannus");
    if (!boss)
        return false;

    return true;
}
