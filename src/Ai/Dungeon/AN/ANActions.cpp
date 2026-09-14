/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ANActions.h"
#include "Playerbots.h"
#include "MotionMaster.h"
#include "Log.h"
#include "PlayerbotAIConfig.h"
#include <algorithm>
#include <cmath>

namespace
{
// 取目标点的 vmap 地面高度并要求与脚下同一层（平台是平的，z 221–224）。
// 不用 Map::CheckCollisionAndGetValidCoords / MoveTo 的路径搜索：run 463/464 证明这一带 mmap 会把终点吸附到
// 平台下层（沿 spline 匀速走进深渊）或上层（为了到 6 码外的点绕行 80 码跑上北面坡道）。
bool ResolveGround(Player* bot, float x, float y, float& z)
{
    z = bot->GetMapHeight(x, y, bot->GetPositionZ() + 2.0f);
    return z > INVALID_HEIGHT && std::fabs(z - bot->GetPositionZ()) <= 3.0f;
}

// 场地内 5–20 码的短距离移动：直线 spline，不经 mmap（generatePath=false）。返回预计耗时（毫秒）。
float MoveStraightNoPath(Player* bot, float x, float y, float z)
{
    MotionMaster* mm = bot->GetMotionMaster();
    mm->Clear();
    mm->MovePoint(0, x, y, z, FORCED_MOVEMENT_NONE, 0.f, 0.f, /*generatePath*/ false, /*forceDestination*/ false);
    float const speed = bot->GetSpeed(MOVE_RUN);
    return speed > 0.1f ? 1000.0f * bot->GetExactDist(x, y, z) / speed : 1000.0f;
}

void LogArenaMove(PlayerbotAI* botAI, Player* bot, char const* action, char const* result, float x, float y, float z)
{
    if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && botAI->HasGameClientMaster()))
        LOG_DEBUG("playerbots", "an-move bot={} action={} result={} from=({:.1f},{:.1f},{:.1f}) to=({:.1f},{:.1f},{:.1f})",
                  bot->GetName(), action, result, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), x, y, z);
}
}  // namespace

bool AttackWebWrapAction::isUseful() { return !botAI->IsHeal(bot); }
bool AttackWebWrapAction::Execute(Event /*event*/)
{
    Unit* webWrap = nullptr;

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_WEB_WRAP)
        {
            webWrap = unit;
            break;
        }
    }
    if (!webWrap || AI_VALUE(Unit*, "current target") == webWrap)
    {
        return false;
    }

    return Attack(webWrap);
}

bool WatchersTargetAction::isUseful() { return !botAI->IsHeal(bot); }
bool WatchersTargetAction::Execute(Event /*event*/)
{
    // Always prioritise web wraps
    Unit* currTarget = AI_VALUE(Unit*, "current target");
    if (currTarget && currTarget->GetEntry() == NPC_WEB_WRAP) { return false; }

    // Do not search all units in range!
    // There are many adds we don't want to aggro in close proximity,
    // only check in-combat adds now.
    GuidVector attackers = AI_VALUE(GuidVector, "attackers");
    Unit* priorityTargets[4] = {nullptr, nullptr, nullptr, nullptr};

    for (auto& attacker : attackers)
    {
        Unit* npc = botAI->GetUnit(attacker);
        if (!npc)
        {
            continue;
        }
        switch (npc->GetEntry())
        {
            // Focus skirmishers first
            case NPC_WATCHER_SKIRMISHER:
                priorityTargets[0] = npc;
                break;
            // Then shadowcaster. This doesn't work so well for the shadowcaster
            // + skirmisher pack - ideally we would kill the watcher second.
            // But don't want to make this unnecessarily complex and rigid...
            // Will revisit if this causes problems in heroic.
            case NPC_WATCHER_SHADOWCASTER:
                priorityTargets[1] = npc;
                break;
            // Named watcher next
            case NPC_WATCHER_SILTHIK:
            case NPC_WATCHER_GASHRA:
            case NPC_WATCHER_NARJIL:
                priorityTargets[2] = npc;
                break;
            // Warrior last
            case NPC_WATCHER_WARRIOR:
                priorityTargets[3] = npc;
                break;
        }
    }

    for (Unit* target : priorityTargets)
    {
        // Attack the first valid split target in the priority list
        if (target)
        {
            if (currTarget != target)
            {
                // bot->Yell("ATTACKING "+target->GetName(), LANG_UNIVERSAL);
                return Attack(target);
            }
            // Don't continue loop here, the target exists so we don't
            // want to move down the prio list. We just don't need to send attack
            // command again, just return false and exit the loop that way
            return false;
        }
    }

    return false;
}

bool AnubarakDodgeImpaleAction::Execute(Event /*event*/)
{
    Unit* spike = FindNearestImpaleSpike(bot, kImpaleTriggerRadius);
    if (!spike)
        return false;

    float const distance = bot->GetExactDist2d(spike->GetPosition());
    float const step = kImpaleSafeDistance - distance;
    if (step <= 0.0f)
        return false;

    // 朝「尖刺 -> 自己」的方向走开。Move() 自带碰撞与坐标校验，不会把 bot 推下平台。
    return Move(spike->GetAngle(bot), step);
}

bool AnubarakDodgePoundAction::isUseful()
{
    // 坦克不动：它得维持仇恨，而且正面本来就是它，扛得住实测 17,153。
    if (botAI->IsTank(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss)
        return false;

    // 距离按 core 的口径：WorldObjectSpellAreaTargetCheck -> IsWithinDist3d(pos, range) = 三维距离 < range + 目标 GetObjectSize()。
    // run 467/1 之前用二维精确距离 <= 15，站在 15–17 码的人一律 USELESS，然后被 27k 打死。
    // **不再看朝向**：践踏目标是 10 码内随机一人，boss 读条时会朝它转（FocusTarget），服务端 GetOrientation 与锥的真实方向
    // 不一致——run 471/1 盗贼在"背后"被判 8 次 USELESS 后被 30k 秒杀（457/3、459/1、466/1 同）。践踏每 ~75 秒一次、
    // 读条 3.2 秒，退出 15 码只损失约 3 秒近战输出，代价远小于一次秒杀。
    // 治疗只在可能成为践踏目标的 10 码内才躲：run 472/2、472/4 牧师在坦克吃 16–20k 践踏的同一刻跑位、0 治疗，坦克倒下。
    float const reach = botAI->IsHeal(bot) ? kPoundTargetRadius : kPoundConeRadius;
    return bot->GetExactDist(boss) <= reach + bot->GetObjectSize();
}

bool AnubarakDodgePoundAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss) { return false; }

    float const distance = bot->GetExactDist2d(boss->GetPosition());

    // ±60° 的宽锥下横移出锥要走 d·tan(60°)≈1.7d，径向退出 15 码锥长只要 15-d：d>6 时径向更短，
    // 而且退到 15 码外以后践踏永远打不到。近战（或贴脸）则绕到 boss 背后——锥只朝前。
    float x, y, z;
    if (botAI->IsMelee(bot) || distance < 6.0f)
    {
        float const back = boss->GetOrientation() + float(M_PI);
        x = boss->GetPositionX() + std::cos(back) * kPoundMeleeBehindDistance;
        y = boss->GetPositionY() + std::sin(back) * kPoundMeleeBehindDistance;
        if (x < kArenaSafeMinX)
            x = kArenaSafeMinX;
    }
    else if (!AnubarakKeepRangeAction::PickPointAwayFromBoss(boss, bot, kPoundConeRadius + 3.0f, x, y, z))
    {
        LogArenaMove(botAI, bot, getName().c_str(), "no_candidate", boss->GetPositionX(), boss->GetPositionY(), 0.f);
        return false;
    }

    if (!botAI->CanMove())
    {
        LogArenaMove(botAI, bot, getName().c_str(), "cannot_move", x, y, 0.f);
        return false;
    }
    if (!(botAI->IsMelee(bot) || distance < 6.0f) || ResolveGround(bot, x, y, z))
    {
        // 远程分支的点已在 PickPointAwayFromBoss 里校验过地面；近战分支在这里校验
    }
    else
    {
        LogArenaMove(botAI, bot, getName().c_str(), "ground_reject", x, y, z);
        return false;
    }
    float const delay = MoveStraightNoPath(bot, x, y, z);
    RecordLastMovement(bot->GetMapId(), x, y, z, delay, MovementPriority::MOVEMENT_FORCED);
    LogArenaMove(botAI, bot, getName().c_str(), "ok", x, y, z);
    return true;
}

bool AnubarakRimGuardAction::Execute(Event /*event*/)
{
    if (bot->isMoving() && AI_VALUE(LastMovement&, "last movement").issuer == getName())
        return false;  // 已经在往回走
    if (!botAI->CanMove())
    {
        LogArenaMove(botAI, bot, getName().c_str(), "cannot_move", 0.f, 0.f, 0.f);
        return false;
    }
    // 沿"中心->自己"的半径拉回到 kArenaGuardRadius；那个点不合法就退而求其次只把 x 拉回西沿护栏线。
    float dx = bot->GetPositionX() - kArenaCenterX;
    float dy = bot->GetPositionY() - kArenaCenterY;
    float const len = std::sqrt(dx * dx + dy * dy);
    float x, y, z;
    bool ok = false;
    if (len > 0.5f)
    {
        x = kArenaCenterX + dx / len * kArenaGuardRadius;
        y = kArenaCenterY + dy / len * kArenaGuardRadius;
        ok = x >= kArenaGuardX && ResolveGround(bot, x, y, z);
    }
    if (!ok)
    {
        x = std::max(kArenaGuardX, bot->GetPositionX());
        y = bot->GetPositionY();
        ok = ResolveGround(bot, x, y, z);
    }
    if (!ok)
    {
        LogArenaMove(botAI, bot, getName().c_str(), "ground_reject", x, y, z);
        return false;
    }
    float const delay = MoveStraightNoPath(bot, x, y, z);
    RecordLastMovement(bot->GetMapId(), x, y, z, delay, MovementPriority::MOVEMENT_FORCED);
    LogArenaMove(botAI, bot, getName().c_str(), "ok", x, y, z);
    return true;
}

bool AnubarakKeepRangeAction::PickPointAwayFromBoss(Unit* boss, Player* bot, float radius, float& x, float& y, float& z)
{
    float const bx = boss->GetPositionX();
    float const by = boss->GetPositionY();
    float base = std::atan2(bot->GetPositionY() - by, bot->GetPositionX() - bx);
    if (bot->GetExactDist2d(boss->GetPosition()) < 0.5f)  // 贴脸时沿 boss 背后方向退
        base = boss->GetOrientation() + float(M_PI);

    static constexpr int kSteps[] = {0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5, 6, -6};
    for (int k : kSteps)
    {
        float const ang = base + float(k) * 15.0f * float(M_PI) / 180.0f;
        x = bx + std::cos(ang) * radius;
        y = by + std::sin(ang) * radius;
        if (x < kArenaGuardX)
            continue;
        float const cx = x - kArenaCenterX;
        float const cy = y - kArenaCenterY;
        if (cx * cx + cy * cy > kArenaSafeRadius * kArenaSafeRadius)
            continue;
        if (ResolveGround(bot, x, y, z))
            return true;
    }
    return false;
}

bool AnubarakKeepRangeAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss)
        return false;
    if (bot->isMoving() && AI_VALUE(LastMovement&, "last movement").issuer == getName())
        return false;  // 已经在退，别每 tick 重发一条新 spline（run 463 单场 31–36 次）
    if (!botAI->CanMove())
    {
        LogArenaMove(botAI, bot, getName().c_str(), "cannot_move", 0.f, 0.f, 0.f);
        return false;
    }
    float x, y, z;
    float const keepTarget = botAI->IsHeal(bot) ? kHealerKeepTarget : kRangedKeepTarget;
    if (!PickPointAwayFromBoss(boss, bot, keepTarget, x, y, z))
    {
        LogArenaMove(botAI, bot, getName().c_str(), "no_candidate", boss->GetPositionX(), boss->GetPositionY(), 0.f);
        return false;
    }
    float const delay = MoveStraightNoPath(bot, x, y, z);
    RecordLastMovement(bot->GetMapId(), x, y, z, delay, MovementPriority::MOVEMENT_FORCED);
    LogArenaMove(botAI, bot, getName().c_str(), "ok", x, y, z);
    return true;
}
