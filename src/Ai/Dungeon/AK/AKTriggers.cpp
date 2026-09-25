/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AKTriggers.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

bool NadoxGuardianTrigger::IsActive()
{
    if (botAI->IsHeal(bot)) { return false; }

    Unit* boss = AI_VALUE2(Unit*, "find target", "elder nadox");
    Unit* guardian = AI_VALUE2(Unit*, "find target", "ahn'kahar guardian");

    return boss && guardian;
}

bool JedogaVolunteerTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "jedoga shadowseeker");
    if (!boss) { return false; }

    // Volunteer is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_TWILIGHT_VOLUNTEER)
        {
            return true;
        }
    }
    return false;
}

Unit* FindAmanitarHealthyMushroom(PlayerbotAI* botAI, Unit* boss)
{
    if (!boss)
        return nullptr;

    Unit* best = nullptr;
    float bestDist = 0.0f;
    // Mushrooms are passive and never threaten anyone, so they are not in the threat-based
    // target values; scan the no-LOS candidate list by entry (same as the Jedoga volunteers).
    GuidVector targets = botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets no los")->Get();
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->GetEntry() != NPC_HEALTHY_MUSHROOM)
            continue;

        float const dist = boss->GetExactDist2d(unit);
        if (!best || dist < bestDist)
        {
            best = unit;
            bestDist = dist;
        }
    }
    return best;
}

bool AmanitarMiniTrigger::IsActive()
{
    // Only damage dealers benefit: Mini/Potent Fungus change damage done, the tank must stay
    // on the boss and healers are unaffected.
    if (!botAI->IsDps(bot) || !bot->HasAura(SPELL_MINI))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "amanitar");
    if (!boss || !boss->IsInCombat())
        return false;

    return FindAmanitarHealthyMushroom(botAI, boss) != nullptr;
}

bool ShadowCrashTrigger::IsActive()
{
    Unit* unit = AI_VALUE2(Unit*, "find target", "forgotten one");
    if (!unit) { return false; }

    return !botAI->IsMelee(bot);
}
