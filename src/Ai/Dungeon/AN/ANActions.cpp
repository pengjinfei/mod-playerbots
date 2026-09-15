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
// 直线路径 bot→(x,y) 距 boss 中心的最近距离。躲踏/保距的候选点本身可能安全，但走过去的路会穿过锥：
// run 488/4 萨满在西沿 15.8 码、锥外 83°，径向候选被护栏挡掉后选了侧向 18 码点，直线路过 13.9 码处、偏轴 10° 时被 23.5k 秒杀。
float PathMinDistToBoss(Player* bot, Unit* boss, float x, float y)
{
    float const ax = bot->GetPositionX(), ay = bot->GetPositionY();
    float const dx = x - ax, dy = y - ay;
    float const len2 = dx * dx + dy * dy;
    float t = 0.f;
    if (len2 > 0.01f)
        t = std::clamp(((boss->GetPositionX() - ax) * dx + (boss->GetPositionY() - ay) * dy) / len2, 0.f, 1.f);
    float const px = ax + dx * t, py = ay + dy * t;
    return boss->GetExactDist2d(px, py);
}

// bot 相对 boss 朝向的偏轴角（0 = 正前方，π = 正后方）。
float OffAxisAngle(Unit* boss, Player* bot)
{
    float const facing = Position::NormalizeOrientation(boss->GetOrientation());
    float const toBot = std::atan2(bot->GetPositionY() - boss->GetPositionY(), bot->GetPositionX() - boss->GetPositionX());
    float off = std::fabs(Position::NormalizeOrientation(toBot - facing));
    return off > float(M_PI) ? 2.0f * float(M_PI) - off : off;
}
}  // namespace

// 给 ANTriggers 用的公开包装：bot 相对 boss 朝向的偏轴角（0 = 正前，π = 正后）。
float AnubarakOffAxisAngle(Unit* boss, Player* bot) { return OffAxisAngle(boss, bot); }

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

    // 原来走共享层 Move()（CheckCollisionAndGetValidCoords + 寻路 MoveTo），run 483–486 四个非坦克 FAILED 是 OK 的 3 倍
    // （法师 61:10），穿刺每场每人中 1–8 次、7–8k 一记，法师被毒箭齐射打残后就是它收的。改成与躲踏/保距同一套：
    // 直线 spline、同层地面校验；首选沿「尖刺→自己」方向，落点出护栏就换 ±45°/±90°。
    if (bot->isMoving() && AI_VALUE(LastMovement&, "last movement").issuer == getName())
        return false;  // 已经在走开
    if (!botAI->CanMove())
    {
        LogArenaMove(botAI, bot, getName().c_str(), "cannot_move", 0.f, 0.f, 0.f);
        return false;
    }
    float const base = spike->GetAngle(bot);
    static constexpr float kTurns[] = {0.f, 0.785f, -0.785f, 1.571f, -1.571f};
    for (float turn : kTurns)
    {
        float const ang = base + turn;
        float const x = bot->GetPositionX() + std::cos(ang) * step;
        float const y = bot->GetPositionY() + std::sin(ang) * step;
        if (x < kArenaGuardX && x < bot->GetPositionX())
            continue;  // 西沿护栏：不比现在更靠西即可——站在准备点 (527,248) 的人否则永远没有候选点（run 487 no_candidate 14 次）
        float const cx = x - kArenaCenterX;
        float const cy = y - kArenaCenterY;
        if (cx * cx + cy * cy > kArenaSafeRadius * kArenaSafeRadius)
            continue;
        float z;
        if (!ResolveGround(bot, x, y, z))
            continue;
        float const delay = MoveStraightNoPath(bot, x, y, z);
        RecordLastMovement(bot->GetMapId(), x, y, z, delay, MovementPriority::MOVEMENT_FORCED);
        LogArenaMove(botAI, bot, getName().c_str(), "ok", x, y, z);
        return true;
    }
    LogArenaMove(botAI, bot, getName().c_str(), "no_candidate", spike->GetPositionX(), spike->GetPositionY(), 0.f);
    return false;
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
    if (bot->GetExactDist(boss) > reach + bot->GetObjectSize())
        return false;
    // 读条期朝向可信（Spell::prepare → FocusTarget → SetInFront），已经站在 ±(60°+15°) 锥外的人不要动：
    // run 488/4 萨满在锥外 83° 被躲踏动作挪进锥里打死。（第七轮"不再看朝向"是在锥角还按 24° 算的时候下的结论。）
    return OffAxisAngle(boss, bot) <= kPoundConeArc / 2.0f + 15.0f * float(M_PI) / 180.0f;
}

bool AnubarakDodgePoundAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss) { return false; }

    float const distance = bot->GetExactDist2d(boss->GetPosition());

    // ±60° 的宽锥下横移出锥要走 d·tan(60°)≈1.7d，径向退出 15 码锥长只要 15-d：d>6 时径向更短，
    // 而且退到 15 码外以后践踏永远打不到。近战（或贴脸）则绕到 boss 背后——锥只朝前。
    // 读条期 boss 的服务端朝向就是锥轴：Spell::prepare → Creature::FocusTarget → SetInFront(践踏目标)，DisableRotate 只关客户端转向。
    // run 483/1、483/5 盗贼站在正面 10.7 / 15 码，"绕到背后 5 码"要穿过整个正面锥（20 码、2.9 秒），践踏落地时还在锥里被 31–32k 秒杀。
    // 正面且 ≥8 码的近战改为径向退出（15→18 只要 3 码）；贴近或已在侧后方的才绕背后。
    float const facing = Position::NormalizeOrientation(boss->GetOrientation());
    float const offAxis = OffAxisAngle(boss, bot);
    bool const inFront = offAxis <= kPoundConeArc / 2.0f + 15.0f * float(M_PI) / 180.0f;  // 60° 半锥 + 15° 余量
    bool const goBehind = (botAI->IsMelee(bot) || distance < 6.0f) && (distance < 8.0f || !inFront);

    float x, y, z;
    bool behindOk = false;
    if (goBehind)
    {
        float const back = facing + float(M_PI);
        x = boss->GetPositionX() + std::cos(back) * kPoundMeleeBehindDistance;
        y = boss->GetPositionY() + std::sin(back) * kPoundMeleeBehindDistance;
        // 不再把 x 夹到 kArenaSafeMinX：run 488/3 boss 被拉到 x=533.7，盗贼在准备点 (527,248) 本来就在背后，背后点 (528.7,250) 被夹成 (533,250)
        // ——正好是 boss 脚下，盗贼跑上去吃了 33.8k。护栏同样用"不比现在更靠西"；不合法就退到径向分支。
        behindOk = x >= kArenaGuardX || x >= bot->GetPositionX();
    }
    if (!behindOk &&
        !AnubarakKeepRangeAction::PickPointAwayFromBoss(boss, bot, kPoundConeRadius + 3.0f, x, y, z, /*keepDistance*/ true))
    {
        LogArenaMove(botAI, bot, getName().c_str(), "no_candidate", boss->GetPositionX(), boss->GetPositionY(), 0.f);
        return false;
    }

    if (!botAI->CanMove())
    {
        LogArenaMove(botAI, bot, getName().c_str(), "cannot_move", x, y, 0.f);
        return false;
    }
    if (!behindOk || ResolveGround(bot, x, y, z))
    {
        // 径向分支的点已在 PickPointAwayFromBoss 里校验过地面；背后分支在这里校验
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

bool AnubarakMeleeBehindAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss)
        return false;

    if (bot->isMoving() && AI_VALUE(LastMovement&, "last movement").issuer == getName())
        return false;  // 已经在绕过去了

    if (!botAI->CanMove())
        return false;

    float const back = Position::NormalizeOrientation(boss->GetOrientation()) + float(M_PI);
    float x = boss->GetPositionX() + std::cos(back) * kPoundMeleeBehindDistance;
    float y = boss->GetPositionY() + std::sin(back) * kPoundMeleeBehindDistance;
    float z = boss->GetPositionZ();

    // 与躲踏同一条西沿护栏：不比现在更靠西（run 488/3 的教训，见 AnubarakDodgePoundAction）
    if (!(x >= kArenaGuardX || x >= bot->GetPositionX()))
    {
        LogArenaMove(botAI, bot, getName().c_str(), "west_reject", x, y, z);
        return false;
    }

    if (!ResolveGround(bot, x, y, z))
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

bool AnubarakKeepRangeAction::PickPointAwayFromBoss(Unit* boss, Player* bot, float radius, float& x, float& y, float& z,
                                                     bool keepDistance)
{
    // keepDistance：走过去的直线不得比现在更靠近 boss（躲踏用；保距时 boss 没在读条，路过近处无妨）
    float const nowDist = bot->GetExactDist2d(boss->GetPosition());
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
        if (x < kArenaGuardX && x < bot->GetPositionX())
            continue;  // 同上：不比现在更靠西即可，否则 boss 被拉到西沿时保距/躲踏全是 no_candidate（run 483 47 次）
        float const cx = x - kArenaCenterX;
        float const cy = y - kArenaCenterY;
        if (cx * cx + cy * cy > kArenaSafeRadius * kArenaSafeRadius)
            continue;
        if (keepDistance && PathMinDistToBoss(bot, boss, x, y) < std::min(nowDist, kPoundConeRadius + 1.0f) - 0.5f)
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

bool AnubarakFocusVenomancerAction::Execute(Event /*event*/)
{
    auto* prioritized = botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets");
    if (Unit* want = FindVenomancerToFocus(botAI, bot))
    {
        prioritized->Set({want->GetGUID()});
        if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && botAI->HasGameClientMaster()))
            LOG_DEBUG("playerbots", "an-focus bot={} venomancer={} dist={:.1f}", bot->GetName(),
                      want->GetGUID().GetCounter(), bot->GetExactDist(want));
        return true;
    }
    prioritized->Reset();
    if (!sPlayerbotAIConfig.logInGroupOnly || (bot->GetGroup() && botAI->HasGameClientMaster()))
        LOG_DEBUG("playerbots", "an-focus bot={} cleared", bot->GetName());
    return true;
}
