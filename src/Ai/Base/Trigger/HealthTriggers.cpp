/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "HealthTriggers.h"
#include "Playerbots.h"

bool HealthInRangeTrigger::IsActive()
{
    return ValueInRangeTrigger::IsActive() && !AI_VALUE2(bool, "dead", GetTargetName());
}

float HealthInRangeTrigger::GetValue() { return AI_VALUE2(uint8, "health", GetTargetName()); }

namespace
{
    // 队里的治疗蓝低于此值（或已阵亡）才接手，别抢正常治疗的活
    constexpr uint8 kOffhealHealerManaPct = 20;
    // 目标血量低于此值才值得花一发治疗
    constexpr uint8 kOffhealTargetHealthPct = 65;
    // 自己蓝低于此值就别补，留给输出
    constexpr uint8 kOffhealSelfManaPct = 25;
}

std::string const OffhealSpellName(Player* bot)
{
    switch (bot->getClass())
    {
        case CLASS_SHAMAN:  return "healing wave";    // 每点法力治疗量最高
        case CLASS_PALADIN: return "holy light";
        case CLASS_DRUID:   return "healing touch";
        case CLASS_PRIEST:  return "greater heal";
        default:            return "";
    }
}

bool PartyNeedsOffhealTrigger::IsActive()
{
    if (!bot->IsAlive() || !bot->IsInCombat())
        return false;

    // 治疗自己不需要这条；坦克跑去补治疗等于丢仇恨。
    if (PlayerbotAI::IsHeal(bot) || PlayerbotAI::IsTank(bot))
        return false;

    std::string const spell = OffhealSpellName(bot);
    if (spell.empty())
        return false;

    if (AI_VALUE2(uint8, "mana", "self target") < kOffhealSelfManaPct)
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // 队里还有能打的治疗就不接手（阵亡或没蓝都算"不能打"）
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !PlayerbotAI::IsHeal(member))
            continue;

        if (member->IsAlive() && member->GetPowerPct(POWER_MANA) >= kOffhealHealerManaPct)
            return false;
    }

    Unit* target = AI_VALUE(Unit*, "party member to heal");
    if (!target || !target->IsAlive() || target->GetHealthPct() > kOffhealTargetHealthPct)
        return false;

    return botAI->CanCastSpell(spell, target);
}

bool PartyMemberDeadTrigger::IsActive() { return GetTarget(); }

bool CombatPartyMemberDeadTrigger::IsActive() { return GetTarget(); }

bool DeadTrigger::IsActive() { return AI_VALUE2(bool, "dead", GetTargetName()); }

bool AoeHealTrigger::IsActive() { return AI_VALUE2(uint8, "aoe heal", type) >= count; }

bool HealerLowManaTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    return target->GetPowerPct(POWER_MANA) < sPlayerbotAIConfig.lowMana;
}

bool AoeInGroupTrigger::IsActive()
{
    int32 member = botAI->GetNearGroupMemberCount();
    if (member < 5)
        return false;
    int threshold = member * 0.5;
    if (member <= 5)
        threshold = 3;
    else if (member <= 10)
        threshold = std::min(threshold, 5);
    else if (member <= 25)
        threshold = std::min(threshold, 10);
    else
        threshold = std::min(threshold, 15);

    return AI_VALUE2(uint8, "aoe heal", type) >= threshold;
}
