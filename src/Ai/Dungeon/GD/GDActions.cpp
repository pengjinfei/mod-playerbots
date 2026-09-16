/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GDActions.h"
#include "Playerbots.h"
#include "TemporarySummon.h"
#include <limits>

bool AvoidPoisonNovaAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "slad'ran");
    if (!boss) { return false; }

    float distance = bot->GetExactDist2d(boss->GetPosition());
    float radius = 15.0f;
    float distanceExtra = 2.0f;

    if (distance < radius + distanceExtra)
    {
        return MoveAway(boss, radius + distanceExtra - distance);
    }

    return false;
}

// 找出「这个包裹困住的是谁」。包裹（29742）是 55126 打在玩家自己身上召出来的，
// 所以优先读召唤者；读不到就退回「最近的队友」近似。
Player* AttackSnakeWrapAction::ResolveWrapVictim(Unit* wrap, char const** how)
{
    if (how)
        *how = "none";

    ObjectGuid ownerGuid = wrap->GetOwnerGUID();
    char const* path = "owner";
    if (!ownerGuid)
    {
        ownerGuid = wrap->GetCreatorGUID();
        path = "creator";
    }
    if (!ownerGuid)
        if (TempSummon* summon = wrap->ToTempSummon())
        {
            ownerGuid = summon->GetSummonerGUID();
            path = "summoner";
        }

    if (ownerGuid)
        if (Player* owner = ObjectAccessor::FindPlayer(ownerGuid))
        {
            if (how)
                *how = path;
            return owner;
        }

    // 退路：包裹就长在被困者身上，取最近的队友
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    Player* nearest = nullptr;
    float nearestDist = 8.0f;  // 超过 8 码就不当作「长在他身上」
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive() || member->GetMapId() != wrap->GetMapId())
            continue;
        float const dist = member->GetExactDist2d(wrap);
        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearest = member;
        }
    }
    if (how && nearest)
        *how = "nearest";
    return nearest;
}

bool AttackSnakeWrapAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "slad'ran");
    if (!boss) { return false; }

    // 已经在打某个包裹了就别换目标（换目标要四个 tick）。
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    if (currentTarget && currentTarget->GetEntry() == NPC_SNAKE_WRAP && currentTarget->IsAlive())
        return false;

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    // 2026-09-16：原实现取的是列表里**第一个** 29742，既不按距离也不看它困住了谁。
    // 英雄实测（run 582+583 十场，heroic-gd-sladran-disc-n5）：
    //   - 每场 8.3 个包裹，几乎不重叠（同时在场均值 1.07 个、最多 2 个）；
    //   - **45% 落在坦克身上**（缠绕者主要打坦克，Grip 叠 5 层就包一个），
    //     而按「被包裹后到下次施法的间隔 vs 该角色平时间隔的 p90」这个自身对照，
    //     **坦克/法师/盗贼共 64/83 = 77% 的包裹根本没困住人**（间隔 0.0–0.5 秒，
    //     逐秒输出也没有下降）；真正被困的只有牧师那 13 次（中位 2.7s，最长 16.8s）。
    //   - 代价：DPS 有 44–46% 的 tick 在推 `attack snake wrap`，砸进去 67.6k/场。
    //   - 危险的恰恰是那 16%：10 场里 8 场有死亡，**其中 5 场的首死落在
    //     「牧师被包裹后 15 秒」窗口内**（该窗口基础率 16%，富集 3.8 倍；n=8 只作提示）。
    // 所以问题不是「打包裹没用」，是**打错了包裹**。这里改成按优先级选：
    //   困住治疗的 > 困住自己的 > 最近的。
    Unit* best = nullptr;
    float bestScore = std::numeric_limits<float>::max();
    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (!unit || unit->GetEntry() != NPC_SNAKE_WRAP || !unit->IsAlive())
            continue;

        char const* how = "none";
        Player* victim = ResolveWrapVictim(unit, &how);
        float score = bot->GetExactDist2d(unit);
        if (victim)
        {
            if (PlayerbotAI::IsHeal(victim))
                score -= 1000.0f;   // 治疗被困 = 唯一会要命的那一种
            else if (victim == bot)
                score -= 500.0f;    // 困住自己的，自己顺手打掉
        }

        if (score < bestScore)
        {
            bestScore = score;
            best = unit;
        }
    }

    if (!best)
        return false;

    // ⚠ 临时诊断探针（验证「包裹 ↔ 被困者」能不能解出来）。验完删除。
    {
        char const* how = "none";
        Player* victim = ResolveWrapVictim(best, &how);
        LOG_INFO("playerbots", "snakewrap_pick: bot={} wrap={} dist={:.1f} victim={} how={} isHeal={} candidates_score={:.1f}",
                 bot->GetName(), best->GetGUID().ToString(), bot->GetExactDist2d(best),
                 victim ? victim->GetName() : "?", how,
                 victim ? (PlayerbotAI::IsHeal(victim) ? 1 : 0) : -1, bestScore);
    }

    return Attack(best);
}

bool AvoidWhirlingSlashAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gal'darah");
    if (!boss) { return false; }

    float distance = bot->GetExactDist2d(boss->GetPosition());
    float radius = 5.0f;
    float distanceExtra = 2.0f;

    if (distance < radius + distanceExtra)
    {
        if (botAI->IsTank(bot))
        {
            // The boss chases tank during this, leads to jittery stutter-stepping
            // by the tank if we don't pre-move additional range. 2*radius seems ok
            return MoveAway(boss, (2.0f * radius) + distanceExtra - distance);
        }
        // else
        return MoveAway(boss, radius + distanceExtra - distance);
    }

    return false;
}
