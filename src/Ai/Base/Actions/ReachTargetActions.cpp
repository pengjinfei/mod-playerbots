/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ReachTargetActions.h"
#include "Event.h"
#include "Group.h"
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
bool ReachTargetAction::IsChaseLeashed(Unit* target) const
{
    if (!target || !bot->GetGroup() || !bot->IsInCombat())
        return false;

    if (PlayerbotAI::IsTank(bot))
        return false;

    Unit* anchor = AI_VALUE(Unit*, "main tank");
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

std::string const ReachPartyMemberToHealAction::GetTargetName() { return "party member to heal"; }

ReachPartyMemberToResurrectAction::ReachPartyMemberToResurrectAction(PlayerbotAI* botAI)
    : ReachTargetAction(botAI, "reach party member to resurrect", botAI->GetRange("spell"))
{
}

std::string const ReachPartyMemberToResurrectAction::GetTargetName() { return "party member to resurrect"; }
