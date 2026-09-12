/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ANActions.h"
#include "Playerbots.h"

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

    // 只有真站在锥子里的人才需要挪。±12° 的窄锥，大多数时候远程本来就是安全的——
    // 上一版不判角度、让所有人一律往外撤，结果是全队每次践踏都白跑一趟。
    return bot->GetExactDist2d(boss->GetPosition()) <= kPoundConeRadius &&
           boss->HasInArc(kPoundConeArc, bot);
}

bool AnubarakDodgePoundAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'arak");
    if (!boss) { return false; }

    float const distance = bot->GetExactDist2d(boss->GetPosition());
    if (distance < 1.0f)
        return false;

    // 往**侧面**让，不是往后退。
    // 锥子是以 boss 朝向为轴的 ±12° 窄扇形：沿轴线后撤等于一路待在锥子里，要跑满 15 码才出得去
    // （run450 就是这么干的，三场 7 次重击、单次最高 39,254，比不动还差）；
    // 而垂直于 boss->自己 连线横移 L 码，夹角直接加 atan(L / d)，在 d 码处只要 d*tan(12°)≈0.21d
    // 就出锥了。取 0.45d 留一倍余量，并给近战一个 6 码下限（贴脸时 0.21d 太小，抖一下就抖回去）。
    // 横移还有个好处：距离基本不变，远程不掉输出距离、近战不掉仇恨位置。
    float const lateral = std::max(6.0f, distance * 0.45f);

    // 朝偏离锥轴的那一侧让——本来偏左就继续往左，避免横穿锥心。
    float delta = Position::NormalizeOrientation(boss->GetAngle(bot) - boss->GetOrientation());
    if (delta > float(M_PI))
        delta -= 2.0f * float(M_PI);
    float const side = (delta >= 0.0f) ? (float(M_PI) / 2.0f) : (-float(M_PI) / 2.0f);

    return Move(boss->GetAngle(bot) + side, lateral);
}
