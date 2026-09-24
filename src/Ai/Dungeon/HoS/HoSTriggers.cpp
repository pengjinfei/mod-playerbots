/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HoSTriggers.h"
#include "AiObjectContext.h"
#include "Group.h"
#include "Playerbots.h"
#include "Spell.h"

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

Unit* FindTribunalRangedLosRegainTarget(PlayerbotAI* botAI)
{
    Player* bot = botAI->GetBot();
    Group* group = bot->GetGroup();
    AiObjectContext* context = botAI->GetAiObjectContext();
    if (bot->GetMapId() != 599 || !bot->IsAlive() || !bot->IsInCombat() || !group ||
        !PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsHeal(bot) || PlayerbotAI::IsTank(bot) ||
        botAI->HasStrategy("stay", botAI->GetState()) || bot->IsNonMeleeSpellCast(false) ||
        !context->GetValue<GuidVector>("attackers")->Get().empty() || context->GetValue<Unit*>("current target")->Get())
    {
        return nullptr;
    }

    Unit* result = nullptr;
    for (ObjectGuid const guid : context->GetValue<GuidVector>("possible targets no los")->Get())
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->GetMapId() != bot->GetMapId() ||
            (unit->GetEntry() != 27983 && unit->GetEntry() != 27984 && unit->GetEntry() != 27985) ||
            bot->GetExactDist(unit) > 40.0f || bot->IsWithinLOSInMap(unit))
        {
            continue;
        }

        Player* victim = unit->GetVictim() ? unit->GetVictim()->ToPlayer() : nullptr;
        if (!victim || !victim->IsAlive() || victim->GetGroup() != group)
            continue;

        if (!result || bot->GetExactDist(unit) < bot->GetExactDist(result))
            result = unit;
    }

    return result;
}

bool TribunalRangedLosRegainTrigger::IsActive() { return FindTribunalRangedLosRegainTarget(botAI) != nullptr; }

bool TribunalLosReacquireTrigger::IsActive() { return FindTribunalLosReacquireTarget(botAI) != nullptr; }

bool TribunalRangedIdleProbeTrigger::IsActive()
{
    if (bot->GetMapId() != 599 || !bot->IsAlive() || !bot->IsInCombat() || !PlayerbotAI::IsRanged(bot) ||
        PlayerbotAI::IsHeal(bot) || PlayerbotAI::IsTank(bot))
    {
        return false;
    }

    uint32 const now = getMSTime();
    if (lastLogMs && getMSTimeDiff(lastLogMs, now) < 1000)
        return false;
    lastLogMs = now;

    AiObjectContext* context = botAI->GetAiObjectContext();
    GuidVector const attackers = context->GetValue<GuidVector>("attackers")->Get();
    Unit* current = context->GetValue<Unit*>("current target")->Get();

    uint32 adds = 0;
    uint32 addsLos = 0;
    float nearest = -1.0f;
    bool nearestLos = false;
    for (ObjectGuid const guid : context->GetValue<GuidVector>("possible targets no los")->Get())
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() ||
            (unit->GetEntry() != 27983 && unit->GetEntry() != 27984 && unit->GetEntry() != 27985))
        {
            continue;
        }

        bool const los = bot->IsWithinLOSInMap(unit);
        float const dist = bot->GetExactDist(unit);
        ++adds;
        addsLos += los ? 1 : 0;
        if (nearest < 0.0f || dist < nearest)
        {
            nearest = dist;
            nearestLos = los;
        }
    }

    Spell* spell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell)
        spell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

    LOG_INFO("playerbots", "tribunal-ranged-probe app_ms={} bot={} attackers={} adds={} adds_los={} "
        "nearest={:.1f} nearest_los={} current={} current_dist={:.1f} current_los={} victim={} casting={} "
        "moving={} mgen={} pos={:.1f},{:.1f},{:.1f}",
        now, bot->GetName(), attackers.size(), adds, addsLos, nearest, nearestLos,
        current ? current->GetEntry() : 0, current ? bot->GetExactDist(current) : -1.0f,
        current ? bot->IsWithinLOSInMap(current) : false, bot->GetVictim() ? bot->GetVictim()->GetEntry() : 0,
        spell ? spell->m_spellInfo->Id : 0, bot->isMoving(),
        uint32(bot->GetMotionMaster()->GetCurrentMovementGeneratorType()),
        bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());
    return false;
}

bool TribunalSearingGazeTrigger::IsActive()
{
    // The trigger is summoned at the selected player's location and lasts 10s.
    // Only activate inside the observed hit radius; the action then moves clear.
    return bot->IsInCombat() && bot->FindNearestCreature(NPC_SEARING_GAZE_TRIGGER, 5.0f);
}
