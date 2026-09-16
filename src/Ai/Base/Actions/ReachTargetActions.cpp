/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ReachTargetActions.h"
#include "Event.h"
#include "Group.h"
#include "GroupReference.h"
#include "Log.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "ServerFacade.h"

bool ReachTargetAction::Execute(Event /*event*/) { return ReachCombatTo(AI_VALUE(Unit*, GetTargetName()), distance); }

bool ReachTargetAction::isUseful()
{
    // do not move while staying
    if (botAI->HasStrategy("stay", botAI->GetState()))
    {
        return false;
    }

    // do not move while casting
    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr)
    {
        return false;
    }
    Unit* target = GetTarget();
    // float dis = distance + CONTACT_DISTANCE;
    if (!target || bot->IsWithinCombatRange(target, distance))  // ServerFacade::instance().IsDistanceGreaterThan(
        return false;                                            // AI_VALUE2(float, "distance", GetTargetName()), distance)

    if (ChasesEnemy() && IsChaseLeashed(target))
        return false;

    return true;
}

// A non-tank that runs after a dps target leaves the group's fight: freshly spawned adds are valid attackers as soon
// as they threaten anyone in the group (AttackersValue looks up to sightDistance away) and "reach spell"/"reach melee"
// have no upper bound, so casters and melee end up alone at the spawn point and the healer follows them there.
// The fight's footprint is heal range around the main tank: an enemy farther than that is not part of the tank's
// fight yet, so nobody but the tank goes to fetch it. Tanks are exempt because picking adds up is their job.
// 追敌的锚点：非坦克看主坦；坦克看治疗（没有活着的治疗就看最近的活着的队友）。坦克豁免过一版（"接小怪是它的活"），
// run 489/5 证明不行：坦克 37 秒 reach melee 去 100 码外刷新点接守卫，自己站到坡道上 (550,327)，两只守卫走下来 30 秒把留在平台上的四人全杀了。
Unit* ReachTargetAction::ChaseAnchor() const
{
    if (!PlayerbotAI::IsTank(bot))
        return AI_VALUE(Unit*, "main tank");

    Group* group = bot->GetGroup();
    Unit* healer = nullptr;
    Unit* nearest = nullptr;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive() || member->GetMapId() != bot->GetMapId())
            continue;
        if (PlayerbotAI::IsHeal(member) && (!healer || bot->GetExactDist(member) < bot->GetExactDist(healer)))
            healer = member;
        if (!nearest || bot->GetExactDist(member) < bot->GetExactDist(nearest))
            nearest = member;
    }
    return healer ? healer : nearest;
}

bool ReachTargetAction::IsChaseLeashed(Unit* target) const
{
    if (!target || !bot->GetGroup() || !bot->IsInCombat())
        return false;

    Unit* anchor = ChaseAnchor();
    if (!anchor || anchor == bot || !anchor->IsAlive() || anchor->GetMapId() != bot->GetMapId())
        return false;

    float const leash = botAI->GetRange("heal");
    float const anchorDist = anchor->GetExactDist(target);
    if (anchorDist <= leash)
        return false;

    LOG_DEBUG("playerbots", "reach-leash bot={} action={} target={} refused: anchor={} anchorDist={:.1f} leash={:.1f} "
              "toTarget={:.1f}",
              bot->GetName(), name, target->GetName(), anchor->GetName(), anchorDist, leash, bot->GetExactDist(target));
    return true;
}

std::string const ReachTargetAction::GetTargetName() { return "current target"; }

bool CastReachTargetSpellAction::isUseful()
{
    // do not move while staying
    if (botAI->HasStrategy("stay", botAI->GetState()))
    {
        return false;
    }

    return ServerFacade::instance().IsDistanceGreaterThan(AI_VALUE2(float, "distance", "current target"),
                                                (distance + sPlayerbotAIConfig.contactDistance));
}

ReachSpellAction::ReachSpellAction(PlayerbotAI* botAI)
    : ReachTargetAction(botAI, "reach spell", botAI->GetRange("spell"))
{
}

ReachPartyMemberToHealAction::ReachPartyMemberToHealAction(PlayerbotAI* botAI)
    : ReachTargetAction(botAI, "reach party member to heal", botAI->GetRange("heal"))
{
}

std::string const ReachPartyMemberToHealAction::GetTargetName() { return "party member to heal no los"; }

bool ReachPartyMemberToHealAction::isUseful()
{
    if (botAI->HasStrategy("stay", botAI->GetState()))
        return false;

    if (bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr)
        return false;

    Unit* target = GetTarget();
    if (!target)
        return false;

    // 距离够但看不见 —— 基类在这里返回 false，于是治疗原地站着（实测德雷德一段 20 秒：
    // 满血、不动、法杖平A、法力在回，18 码外的坦克被磨死）。这种情况要移动。
    if (!bot->IsWithinLOSInMap(target))
        return true;

    return !bot->IsWithinCombatRange(target, distance);
}

bool ReachPartyMemberToHealAction::Execute(Event /*event*/)
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    if (bot->IsWithinLOSInMap(target))
        return ReachCombatTo(target, distance);

    // 看不见就往目标走：寻路会绕过遮挡，视线一恢复触发器就不再亮、动作自己停下。
    // ⚠ 这里**不能**用 distance（= 治疗距离，40 码）：`ReachCombatTo` 一进门就判
    // 「已经在这个距离内」→ 直接返回 false。第一版写成 max(8, distance*0.5) = 20 码，
    // 而治疗本来就站在 17–19 码，实测 6 次推入 6 次 FAILED，一步没走。
    // 先靠到 8 码；若已经在 10 码内还是看不见（贴着柱子），再靠到 2 码。
    float const closeTo = bot->GetExactDist2d(target) > 10.0f ? 8.0f : 2.0f;
    return ReachCombatTo(target, closeTo);
}

ReachPartyMemberToResurrectAction::ReachPartyMemberToResurrectAction(PlayerbotAI* botAI)
    : ReachTargetAction(botAI, "reach party member to resurrect", botAI->GetRange("spell"))
{
}

std::string const ReachPartyMemberToResurrectAction::GetTargetName() { return "party member to resurrect"; }
