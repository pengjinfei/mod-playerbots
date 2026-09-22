/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoSTriggers.h"
#include "AiObjectContext.h"
#include "Group.h"
#include "Playerbots.h"

bool KrystallusGroundSlamTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "krystallus");
    if (!boss) { return false; }

    // Check both of these... the spell is applied first, debuff later.
    // Neither is active for the full duration so we need to trigger off both
    return bot->HasAura(SPELL_GROUND_SLAM) || bot->HasAura(DEBUFF_GROUND_SLAM);
}

bool SjonnirLightningRingTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "sjonnir the ironshaper");
    if (!boss) { return false; }

    return boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(SPELL_LIGHTNING_RING);
}

Unit* FindTribunalLosReacquireTarget(PlayerbotAI* botAI)
{
    Player* bot = botAI->GetBot();
    Group* group = bot->GetGroup();
    if (!PlayerbotAI::IsTank(bot) || !bot->IsInCombat() || !group ||
        botAI->HasStrategy("stay", botAI->GetState()) ||
        !botAI->GetAiObjectContext()->GetValue<GuidVector>("attackers")->Get().empty() ||
        botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get())
    {
        return nullptr;
    }

    Unit* result = nullptr;
    for (ObjectGuid const guid : botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets no los")->Get())
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->GetMapId() != bot->GetMapId() ||
            (unit->GetEntry() != 27983 && unit->GetEntry() != 27984 && unit->GetEntry() != 27985))
        {
            continue;
        }

        Player* victim = unit->GetVictim() ? unit->GetVictim()->ToPlayer() : nullptr;
        if (!victim || !victim->IsAlive() || victim->GetGroup() != group)
            continue;

        bool nearHealer = false;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member->IsAlive() && member->GetMapId() == bot->GetMapId() &&
                PlayerbotAI::IsHeal(member) && unit->GetExactDist(member) <= botAI->GetRange("heal"))
            {
                nearHealer = true;
                break;
            }
        }
        if (nearHealer && (!result || bot->GetExactDist(unit) < bot->GetExactDist(result)))
            result = unit;
    }

    return result;
}

bool TribunalLosReacquireTrigger::IsActive() { return FindTribunalLosReacquireTarget(botAI) != nullptr; }

bool TribunalSearingGazeTrigger::IsActive()
{
    // The trigger is summoned at the selected player's location and lasts 10s.
    // Only activate inside the observed hit radius; the action then moves clear.
    return bot->IsInCombat() && bot->FindNearestCreature(NPC_SEARING_GAZE_TRIGGER, 5.0f);
}
