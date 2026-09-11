/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NexActions.h"
#include "Group.h"
#include "Log.h"
#include "NexTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace
{
    // 变形术 / 妖术的施法距离（不含 combat reach）。
    constexpr float kCcSpellRange = 30.0f;
    // 控制迟迟不落地时，第一个控制图标打上多久后照样标骷髅（要小于编排层的等待上限）。
    constexpr uint32 kSkullDeadlineMs = 12000;
}

bool MoveFromWhirlwindAction::Execute(Event /*event*/)
{
    Unit* boss = nullptr;
    uint8 faction = bot->GetTeamId();
    float targetDist = 10.0f; // Whirlwind has a range of 8, adding a safety buffer

    switch (bot->GetMap()->GetDifficulty())
    {
        case DUNGEON_DIFFICULTY_NORMAL:
            if (faction == TEAM_ALLIANCE)
                boss = AI_VALUE2(Unit*, "find target", "horde commander");

            else // TEAM_HORDE
                boss = AI_VALUE2(Unit*, "find target", "alliance commander");

            break;
        case DUNGEON_DIFFICULTY_HEROIC:
            if (faction == TEAM_ALLIANCE)
                boss = AI_VALUE2(Unit*, "find target", "commander kolurg");

            else // TEAM_HORDE
                boss = AI_VALUE2(Unit*, "find target", "commander stoutbeard");

            break;
        default:
            break;
    }

    // Ensure boss is valid before accessing its methods
    if (!boss)
        return false;

    float bossDistance = bot->GetExactDist2d(boss->GetPosition());

    // Check if the bot is already at a safe distance
    if (bossDistance > targetDist)
        return false;

    // Move away from the boss to avoid Whirlwind
    return MoveAway(boss, targetDist - bossDistance);
}

bool FirebombSpreadAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand magus telestra");
    float radius = 5.0f;
    float targetDist = radius + 1.0f;
    if (!boss) { return false; }

    GuidVector members = AI_VALUE(GuidVector, "group members");
    for (auto& member : members)
    {
        Unit* unit = botAI->GetUnit(member);
        if (!unit || bot->GetGUID() == member) { continue; }

        if (bot->GetExactDist2d(unit) < targetDist)
            return MoveAway(unit, targetDist);

    }
    return false;
}

bool TelestraSplitTargetAction::isUseful() { return !botAI->IsHeal(bot); }
bool TelestraSplitTargetAction::Execute(Event /*event*/)
{
    GuidVector attackers = AI_VALUE(GuidVector, "attackers");
    Unit* splitTargets[3] = {nullptr, nullptr, nullptr};

    for (auto& attacker : attackers)
    {
        Unit* unit = botAI->GetUnit(attacker);
        if (!unit) { continue; }

        switch (unit->GetEntry())
        {
            // Focus arcane clone first
            case NPC_ARCANE_MAGUS:
                splitTargets[0] = unit;
                break;
            // Then the frost clone
            case NPC_FROST_MAGUS:
                splitTargets[1] = unit;
                break;
            // Fire clone last
            case NPC_FIRE_MAGUS:
                splitTargets[2] = unit;
                break;
        }
    }

    for (Unit* target : splitTargets)
    {
        // Attack the first valid split target in the priority list
        if (target)
        {
            if (AI_VALUE(Unit*, "current target") != target)
                return Attack(target);

            // Don't continue loop here, the target exists so we don't
            // want to move down the prio list. We just don't need to send attack
            // command again, just return false and exit the loop that way
            return false;
        }
    }

    return false;
}

// 坦克排除在外：判据放宽到「场上有存活裂隙」后，转火不再只发生在 boss 免疫的护盾期，
// 坦克若一起转火就会在 boss 可被攻击的窗口丢掉 boss 仇恨。治疗照旧排除。
bool ChaoticRiftTargetAction::isUseful() { return !botAI->IsHeal(bot) && !botAI->IsTank(bot); }
bool ChaoticRiftTargetAction::Execute(Event /*event*/)
{
    // 取最近的存活裂隙，与 ChaoticRiftTrigger 共用同一判据（原来是遍历到第一个同名单位
    // 就停，可能选到已死的或更远的那一个）。
    Unit* chaoticRift = FindNearestChaoticRift(botAI, bot, context);
    if (!chaoticRift || AI_VALUE(Unit*, "current target") == chaoticRift)
        return false;

    return Attack(chaoticRift);
}

bool DodgeSpikesAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ormorok the tree-shaper");
    if (!boss) { return false; }

    return bot->GetExactDist2d(boss) > 0.5f;
}
void TrashCcMarkAction::SetIcon(Group* group, uint8 icon, Unit* target)
{
    ObjectGuid const guid = target ? target->GetGUID() : ObjectGuid::Empty;
    if (group->GetTargetIcon(icon) == guid)
        return;

    // 核心的 SetTargetIcon 会顺手清掉同一目标上的其它图标，所以把骷髅挪到被控的怪上
    // 就等于解除了它的控制图标——控制职业随之停止补控，DPS 随之可以打它。
    group->SetTargetIcon(icon, bot->GetGUID(), guid);
}

bool TrashCcMarkAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    return bot->IsInCombat() ? AdvanceKillOrder(group) : AssignPrePull(group);
}

bool TrashCcMarkAction::AssignPrePull(Group* group)
{
    Unit* pull = botAI->GetUnit(AI_VALUE(ObjectGuid, "pull target"));
    std::vector<Creature*> pack = TrashCcCollectPack(botAI, bot, pull);
    if (pack.empty())
        return false;

    // 治疗 → 施法者 → 其它。前面的分给控制，最后剩下的（最像近战的那只）给坦克。
    std::stable_sort(pack.begin(), pack.end(),
                     [](Creature* a, Creature* b) { return TrashCcPreference(a) < TrashCcPreference(b); });

    auto inPack = [&](Unit* unit) { return unit && std::find(pack.begin(), pack.end(), unit->ToCreature()) != pack.end(); };

    // 至少留一只给坦克。
    size_t const maxCc = pack.size() - 1;
    std::set<ObjectGuid> assigned;
    bool changed = false;

    // 十字（闷棍）先分：盗贼要潜行走过去，只能闷离它最近的那只，否则要穿过整组怪
    //（run405 attempt1/3：闷棍目标在怪堆深处，盗贼走到一半被发现，整组直接进战斗）。
    static uint8 const kIconOrder[] = { TRASH_CC_ICON_CROSS, TRASH_CC_ICON_MOON, TRASH_CC_ICON_SQUARE };
    for (uint8 icon : kIconOrder)
    {
        TrashCcRole const* role = nullptr;
        switch (icon)
        {
            case TRASH_CC_ICON_MOON:   role = TrashCcRoleForClass(CLASS_MAGE);   break;
            case TRASH_CC_ICON_SQUARE: role = TrashCcRoleForClass(CLASS_SHAMAN); break;
            default:                   role = TrashCcRoleForClass(CLASS_ROGUE);  break;
        }

        Player* caster = role ? TrashCcFindCaster(bot, *role) : nullptr;
        if (!caster)
            continue;

        // 已经有合法目标的图标原样保留——控制职业可能已经在读条了，别把它的目标换掉。
        Unit* current = TrashCcIconUnit(botAI, icon);
        if (inPack(current) && TrashCcSpellFits(*role, current->ToCreature()))
        {
            assigned.insert(current->GetGUID());
            continue;
        }

        if (assigned.size() >= maxCc)
            continue;

        // 闷棍：离盗贼最近的那只。羊/妖术：按优先级取第一只在 30 码施法距离内的；一只都不在距离内
        // 就**不分**（准备点必须离怪 >23 码，两只治疗未必都在 30 码内——(509,62) 到两只治疗是
        // 29.4 / 33.3 码；分一只够不着的等于让全队白等 15 秒）。
        Creature* pick = nullptr;
        for (Creature* creature : pack)
        {
            if (assigned.count(creature->GetGUID()) || !TrashCcSpellFits(*role, creature))
                continue;

            if (icon == TRASH_CC_ICON_CROSS)
            {
                if (!pick || caster->GetDistance(creature) < caster->GetDistance(pick))
                    pick = creature;
                continue;
            }

            if (caster->GetDistance(creature) <= kCcSpellRange)
            {
                pick = creature;
                break;
            }
        }
        if (!pick)
            continue;

        SetIcon(group, icon, pick);
        assigned.insert(pick->GetGUID());
        if (PlayerbotAI* casterAI = GET_PLAYERBOT_AI(caster))
            casterAI->GetAiObjectContext()->GetValue<std::string>("rti cc")->Set(role->iconName);
        if (!sPlayerbotAIConfig.logInGroupOnly)
            LOG_DEBUG("playerbots", "trash cc mark: {} -> {} on {} ({}) for {} dist={:.1f}", bot->GetName(),
                      role->iconName, pick->GetName(), pick->GetGUID().GetCounter(), caster->GetName(),
                      caster->GetDistance(pick));
        changed = true;
    }

    // 骷髅：没分给控制的里最靠后的那只（最像近战）。**等分出去的控制全部落地再标**——
    // 骷髅一出现，上游 AttackersValue 就把它当成攻击者，DPS/治疗会在开怪前先动手（run402）。
    // 但也不能等太久：控制放不出来（被沉默、冷却、盗贼被发现）时编排层到点照样开怪，
    // 骷髅缺席会让战斗中的「挪骷髅」把还没被控住的怪当成"都控着了"而放出一只真被控的
    //（run406：门禁超时开怪时骷髅未标，坦克把骷髅挪到了被闷棍的那只上）。
    // 所以：全部落地、或有没被控的怪已进战斗、或第一个控制图标已经打了 12 秒，就标骷髅。
    bool allLanded = true;
    bool engaged = false;
    uint32 oldestIconMs = 0;
    for (ObjectGuid const& guid : assigned)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;
        if (!TrashCcIncapacitated(unit, bot))
        {
            allLanded = false;
            if (unit->IsInCombat())
                engaged = true;
        }
    }
    for (uint8 icon : kIconOrder)
        oldestIconMs = std::max(oldestIconMs, TrashCcIconAgeMs(botAI, icon));
    if (!allLanded && !engaged && oldestIconMs < kSkullDeadlineMs)
    {
        context->GetValue<std::string>("rti")->Set("skull");
        return changed;
    }

    Unit* skull = TrashCcIconUnit(botAI, TRASH_CC_ICON_SKULL);
    if (!inPack(skull) || assigned.count(skull->GetGUID()))
    {
        skull = nullptr;
        for (auto it = pack.rbegin(); it != pack.rend(); ++it)
            if (!assigned.count((*it)->GetGUID()))
            {
                skull = *it;
                break;
            }

        if (skull)
        {
            SetIcon(group, TRASH_CC_ICON_SKULL, skull);
            changed = true;
        }
    }

    context->GetValue<std::string>("rti")->Set("skull");
    return changed;
}

bool TrashCcMarkAction::AdvanceKillOrder(Group* group)
{
    // 先找活着的、没有控制图标的攻击者，离坦克最近的优先。
    Unit* best = nullptr;
    for (ObjectGuid const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || !unit->IsCreature())
            continue;

        bool ccIcon = false;
        for (uint8 icon : { TRASH_CC_ICON_MOON, TRASH_CC_ICON_SQUARE, TRASH_CC_ICON_CROSS })
            if (group->GetTargetIcon(icon) == guid)
                ccIcon = true;
        if (ccIcon)
            continue;

        if (!best || bot->GetDistance(unit) < bot->GetDistance(best))
            best = unit;
    }

    if (best)
    {
        SetIcon(group, TRASH_CC_ICON_SKULL, best);
        return true;
    }

    // 其次：有控制图标但**此刻没被控住**、而且在打我们的（控制没放出来/被打掉了还没补上）。
    // 它本来就在战斗里，先杀它不破坏任何控制。
    for (uint8 icon : { TRASH_CC_ICON_CROSS, TRASH_CC_ICON_SQUARE, TRASH_CC_ICON_MOON })
    {
        Unit* unit = TrashCcIconUnit(botAI, icon);
        if (unit && unit->IsInCombat() && !TrashCcIncapacitated(unit, bot))
        {
            SetIcon(group, TRASH_CC_ICON_SKULL, unit);
            return true;
        }
    }

    // 只剩被控的：闷棍（只能用一次）→ 妖术（45 秒冷却）→ 变形术（可反复）依次放出来打。
    for (uint8 icon : { TRASH_CC_ICON_CROSS, TRASH_CC_ICON_SQUARE, TRASH_CC_ICON_MOON })
    {
        if (Unit* unit = TrashCcIconUnit(botAI, icon))
        {
            SetIcon(group, TRASH_CC_ICON_SKULL, unit);
            return true;
        }
    }

    return false;
}

bool TrashCcSapAction::isUseful()
{
    TrashCcRole const* role = TrashCcRoleForClass(CLASS_ROGUE);
    return role && TrashCcCastTarget(botAI, bot, *role) != nullptr;
}

bool TrashCcSapAction::Execute(Event /*event*/)
{
    TrashCcRole const* role = TrashCcRoleForClass(CLASS_ROGUE);
    Unit* target = role ? TrashCcCastTarget(botAI, bot, *role) : nullptr;
    if (!target)
        return false;

    if (!bot->HasStealthAura())
        return botAI->CastSpell("stealth", bot);

    // 闷棍射程 10 码，核心 CheckRange 还会加上双方的 combat reach（约 +3 码）；同级怪正面约 10.5 码
    // 就能看穿潜行，所以停在 11 码。曾试过「绕到目标背后 10 码」（run412/413 共 4 场闷棍 0 次落地，
    // 此前径直走是 5 场 4 次），绕行路线反而穿过怪堆，已退回径直接近。
    if (bot->GetExactDist2d(target) > 11.5f)
        return MoveNear(target, 11.0f);

    // 先走 CanCastSpell：失败原因会进 Playerbots.log（LogInGroupOnly = 0 时），直接 CastSpell 失败是静默的。
    if (!botAI->CanCastSpell("sap", target))
        return false;

    return botAI->CastSpell("sap", target);
}

bool DodgeSpikesAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ormorok the tree-shaper");
    if (!boss) { return false; }

    return Move(bot->GetAngle(boss), bot->GetExactDist2d(boss) - 0.3f);
}

bool IntenseColdJumpAction::Execute(Event /*event*/)
{
    // This needs improving but maybe it should be done in the playerbot core.
    // Jump doesn't seem to support zero offset (eg. jump on the spot) so need to add a tiny delta.
    // This does a tiny bunnyhop that takes a couple of ms, it doesn't do a natural jump.
    // Adding extra Z offset causes floating, and appears to scale the jump speed based on Z difference.
    // Probably best to revisit once bot movement is improved
    return JumpTo(bot->GetMap()->GetId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ() + 0.01f);
    // bot->GetMotionMaster()->MoveFall();
}
