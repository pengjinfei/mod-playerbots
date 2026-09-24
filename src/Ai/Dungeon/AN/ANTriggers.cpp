/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ANTriggers.h"
#include "ANActions.h"
#include <list>
#include "AiObjectContext.h"
#include "Playerbots.h"
#include "Spell.h"

namespace
{
    // 队里的治疗蓝低于此值（或已阵亡）才接手，别抢正常治疗的活
    constexpr uint8 kOffhealHealerManaPct = 20;
    // 目标血量低于此值才值得花一发治疗波
    constexpr uint8 kOffhealTargetHealthPct = 65;
    // 自己蓝低于此值就别补，留给输出与图腾
    constexpr uint8 kOffhealSelfManaPct = 25;
}

namespace
{
    // 出土计数。夹具一次只跑一个实例，所以用一份按 instanceId 复位的状态即可；
    // boss 血回到 >90%（新的一场或 boss 重置）也复位。
    struct AnubarakEmergeState
    {
        uint32 instanceId = 0;
        uint8 emerges = 0;
        bool submerged = false;
    };

    AnubarakEmergeState g_anubarakEmerge;
}

bool AnubarakAfterThirdEmerge(PlayerbotAI* /*botAI*/, Player* bot)
{
    Creature* boss = bot->FindNearestCreature(kAnubarakEntry, 200.0f);
    if (!boss || !boss->IsAlive())
        return false;

    uint32 const instanceId = bot->GetInstanceId();
    if (g_anubarakEmerge.instanceId != instanceId || boss->GetHealthPct() > 90.0f)
    {
        g_anubarakEmerge.instanceId = instanceId;
        g_anubarakEmerge.emerges = 0;
        g_anubarakEmerge.submerged = false;
    }

    bool const submerged = boss->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
    if (g_anubarakEmerge.submerged && !submerged && g_anubarakEmerge.emerges < 255)
        ++g_anubarakEmerge.emerges;
    g_anubarakEmerge.submerged = submerged;

    return !submerged && g_anubarakEmerge.emerges >= 3;
}

bool AnubarakMeleeFrontTrigger::IsActive()
{
    if (!bot->IsAlive() || !bot->IsInCombat())
        return false;

    // 只管近战输出：坦克必须站正面，治疗/远程本来就在 18-20 码外（实测中位夹角 102-110°）
    if (!botAI->IsMelee(bot) || botAI->IsTank(bot) || botAI->IsHeal(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss || !boss->IsAlive() || boss->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return false;  // 潜地期没有践踏，别干扰打小怪

    if (bot->GetExactDist2d(boss->GetPosition()) > kPoundConeRadius)
        return false;

    // 半锥 60° 再加 10° 余量；绕背后是 180°，不会来回抖
    return AnubarakOffAxisAngle(boss, bot) <= kPoundConeArc / 2.0f + 10.0f * float(M_PI) / 180.0f;
}

bool AnubarakHeroismTrigger::IsActive()
{
    if (!bot->IsAlive() || !bot->IsInCombat() || bot->getClass() != CLASS_SHAMAN)
        return false;

    if (!AnubarakAfterThirdEmerge(botAI, bot))
        return false;

    return botAI->CanCastSpell("heroism", bot) || botAI->CanCastSpell("bloodlust", bot);
}

bool AnubarakOffhealTrigger::IsActive()
{
    if (!bot->IsAlive() || PlayerbotAI::IsHeal(bot) || !bot->IsInCombat())
        return false;

    if (AI_VALUE2(uint8, "mana", "self target") < kOffhealSelfManaPct)
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // 队里还有能打的治疗就不接手
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

    return botAI->CanCastSpell("healing wave", target);
}

bool KrikthirWebWrapTrigger::IsActive()
{
    if (!botAI->IsDps(bot)) { return false; }

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_WEB_WRAP)
        {
            return true;
        }
    }

    return false;
}

bool HadronoxTankAcidTrigger::IsActive()
{
    if (!botAI->IsTank(bot) || !bot->IsAlive() || !bot->IsInCombat())
        return false;
    constexpr uint32 kHadronox = 28921;
    Creature* boss = bot->FindNearestCreature(kHadronox, 40.0f);
    if (!boss || !boss->IsAlive() || !boss->IsInCombat() || boss->GetVictim() != bot)
        return false;
    return bot->HasAura(59419) || bot->HasAura(53400);
}

bool KrikthirWatchersTrigger::IsActive()
{
    if (!botAI->IsDps(bot)) { return false; }

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        // 只在克里克希尔的门厅里生效：「possible targets no los」在哈多诺克斯平台上也能看到楼上活着的
        // 克里克希尔（z≈777 对平台 733），节点（64）每 tick 压过 dps assist(50) 又执行失败，
        // 盗贼 30/31 场哈多诺克斯零输出。要求同层且 60 码内。
        if (unit && unit->GetEntry() == NPC_KRIKTHIR && unit->IsAlive() && bot->GetExactDist2d(unit) <= 60.0f &&
            std::fabs(unit->GetPositionZ() - bot->GetPositionZ()) <= 15.0f)
        {
            return true;
        }
    }
    return false;
}

// bool AnubarakImpaleTrigger::IsActive()
// {
//     Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
//     if (!boss) { return false; }
//     GuidVector triggers = AI_VALUE(GuidVector, "possible triggers");
//     for (auto i = triggers.begin(); i != triggers.end(); i++)
//     {
//         Unit* unit = botAI->GetUnit(*i);

//         if (unit)
//         {
//             bot->Yell("TRIGGER="+unit->GetName(), LANG_UNIVERSAL);
//         }
//     }
//     return false;
// }

Unit* FindNearestImpaleSpike(Player* bot, float range)
{
    // 尖刺带 UNIT_FLAG_NOT_SELECTABLE，不会出现在 "possible targets no los" 里，
    // 所以直接按 entry 就近搜，不走选目标那套。
    return bot->FindNearestCreature(NPC_IMPALE_TARGET, range);
}

bool AnubarakImpaleTrigger::IsActive()
{
    return FindNearestImpaleSpike(bot, kImpaleTriggerRadius) != nullptr;
}

bool AnubarakPoundTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss) { return false; }

    return boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(SPELL_POUND);
}

Unit* FindVenomancerToFocus(PlayerbotAI* botAI, Player* bot)
{
    Unit* anchor = botAI->GetAiObjectContext()->GetValue<Unit*>("main tank")->Get();
    if (!anchor || !anchor->IsAlive())
        anchor = bot;
    std::list<Creature*> found;
    bot->GetCreatureListWithEntryInGrid(found, kVenomancerEntry, kVenomancerSearchRange);
    Unit* best = nullptr;
    float bestDist = 0.f;
    for (Creature* c : found)
    {
        if (!c->IsAlive() || !c->IsInCombat())
            continue;
        if (anchor->GetExactDist(c) > kVenomancerFocusAnchorRange)
            continue;  // 还在坡道上，追敌上限够不到，先别盯
        float const d = bot->GetExactDist(c);
        if (!best || d < bestDist)
        {
            best = c;
            bestDist = d;
        }
    }
    return best;
}

// 与 AnubarakFocusVenomancerAction::Execute 用同一判据：有该盯的目标但没盯上，或盯着的毒疗者已经死了/没了。
bool AnubarakVenomancerFocusTrigger::IsActive()
{
    if (!bot->IsAlive() || !bot->IsInCombat() || botAI->IsTank(bot) || botAI->IsHeal(bot))
        return false;
    GuidVector const current = AI_VALUE(GuidVector, "prioritized targets");
    if (Unit* want = FindVenomancerToFocus(botAI, bot))
        return current.empty() || current.front() != want->GetGUID();
    if (current.empty() || current.front().GetEntry() != kVenomancerEntry)
        return false;
    Unit* held = botAI->GetUnit(current.front());
    return !held || !held->IsAlive();
}

bool AnubarakRimTrigger::IsActive()
{
    if (!bot->IsAlive())
        return false;
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss)
        return false;
    if (bot->GetPositionX() < kArenaSafeMinX)
        return true;
    float const dx = bot->GetPositionX() - kArenaCenterX;
    float const dy = bot->GetPositionY() - kArenaCenterY;
    return dx * dx + dy * dy > kArenaSafeRadius * kArenaSafeRadius;
}

bool AnubarakRangedTooCloseTrigger::IsActive()
{
    // 战斗中才拉开：run 466/1 里开怪前就把牧师挪走，它没进战斗状态，整场留在非战斗引擎里。
    // 远程 DPS 与治疗各有阈值：治疗 10 码内靠躲踏，10–17 码这段过去没人管，run 478/479 五场首死都在这里。
    if (!bot->IsAlive() || !bot->IsInCombat())
        return false;
    float keepDistance;
    if (botAI->IsRangedDps(bot))
        keepDistance = kRangedKeepDistance;
    else if (botAI->IsHeal(bot))
        keepDistance = kHealerKeepDistance;
    else
        return false;
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss || !boss->IsAlive() || boss->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))  // 潜地期没有践踏
        return false;
    return bot->GetExactDist(boss) < keepDistance + bot->GetObjectSize();
}

namespace
{
bool BossCastingPound(PlayerbotAI* botAI, Player* bot)
{
    Unit* boss = botAI->GetAiObjectContext()->GetValue<Unit*>("find target", "anub'arak")->Get();
    return boss && boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(SPELL_POUND);
}
}  // namespace

bool AnubarakPoundTankTrigger::IsActive()
{
    if (!bot->IsAlive() || !botAI->IsTank(bot) || !BossCastingPound(botAI, bot))
        return false;

    Aura* sunder = botAI->GetAura("sunder armor", bot);
    if (sunder && sunder->GetStackAmount() >= kPoundGuardSunderStacks)
        return true;

    // 末段即使破甲不足也开：240-299 秒这档践踏中位 15.5k、最大 20.1k（run 508-514 共 20 场）
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    return boss && boss->GetHealthPct() <= kPoundGuardBossHealthPct;
}

bool AnubarakPoundHealerTrigger::IsActive()
{
    if (!bot->IsAlive() || !botAI->IsHeal(bot) || !BossCastingPound(botAI, bot))
        return false;
    Unit* tank = AI_VALUE(Unit*, "main tank");
    return tank && tank->IsAlive() && bot->IsWithinDistInMap(tank, 40.0f);
}
