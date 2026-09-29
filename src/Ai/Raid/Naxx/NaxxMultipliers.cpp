/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxMultipliers.h"
#include "AttackAction.h"
#include "ChooseTargetActions.h"
#include "DKActions.h"
#include "DruidActions.h"
#include "DruidBearActions.h"
#include "FollowActions.h"
#include "GenericSpellActions.h"
#include "HunterActions.h"
#include "MageActions.h"
#include "MovementActions.h"
#include "NaxxActions.h"
#include "NaxxSpellIds.h"
#include "PaladinActions.h"
#include "PetsAction.h"
#include "PriestActions.h"
#include "ReachTargetActions.h"
#include "RogueActions.h"
#include "ScriptedCreature.h"
#include "ShamanActions.h"
#include "Spell.h"
#include "UseMeetingStoneAction.h"
#include "WarriorActions.h"
#include "WipeAction.h"

float PatchwerkMeleeWaitMultiplier::GetValue(Action* action)
{
    // Only what closes the distance: movement (melee/attack chase included) and charge-like spells.
    if (!dynamic_cast<MovementAction*>(action) && !dynamic_cast<CastReachTargetSpellAction*>(action))
        return 1.0f;
    if (botAI->IsTank(bot) || botAI->IsHeal(bot) || !botAI->IsMelee(bot))
        return 1.0f;

    Unit* boss = AI_VALUE2(Unit*, "find target", "patchwerk");
    if (!boss || !boss->IsInCombat() || boss->IsWithinMeleeRange(bot))
        return 1.0f;

    Group* group = bot->GetGroup();
    if (!group)
        return 1.0f;

    // Wait while a living off-tank exists and none of them is in melee range yet; without one there is nobody to
    // wait for.
    bool offTankAlive = false;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive() || member == boss->GetVictim() ||
            !PlayerbotAI::IsTank(member))
            continue;
        if (boss->IsWithinMeleeRange(member))
            return 1.0f;
        offTankAlive = true;
    }
    return offTankAlive ? 0.0f : 1.0f;
}

float GrobbulusMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss)
        return 1.0f;

    if (dynamic_cast<AvoidAoeAction*>(action))
        return botAI->IsMainTank(bot) ? 0.0f : 1.0f;

    if (dynamic_cast<CombatFormationMoveAction*>(action))
        return 0.0f;

    return 1.0f;
}

float HeiganDanceMultiplier::GetValue(Action* action)
{
    // Cheap action-type checks first; the encounter state is only looked up for actions we may have to block.
    if (dynamic_cast<HeiganDanceAction*>(action) || dynamic_cast<CurePartyMemberAction*>(action) ||
        dynamic_cast<WipeAction*>(action))
        return 1.0f;

    bool repositions = dynamic_cast<CombatFormationMoveAction*>(action) || dynamic_cast<FleeAction*>(action) ||
                       dynamic_cast<CastDisengageAction*>(action) || dynamic_cast<CastBlinkBackAction*>(action);
    bool moves = dynamic_cast<MovementAction*>(action) || dynamic_cast<CastReachTargetSpellAction*>(action);
    auto* spellAction = dynamic_cast<CastSpellAction*>(action);
    bool timedCast = spellAction && !dynamic_cast<CastMeleeSpellAction*>(action);
    if (!repositions && !moves && !timedCast)
        return 1.0f;

    if (!helper.UpdateBossAI())
        return 1.0f;

    // Generic repositioning must never pull a bot off its safe spot or off the platform.
    if (repositions)
        return 0.0f;

    // Ranged bots on the platform during the slow dance are free to act as usual.
    if (!helper.ShouldDance())
        return 1.0f;

    // Dancing: only the dance moves us (charge/intercept/feral charge included - during the fast dance the boss
    // stands in his Plague Cloud). Everything that is not a cast is fine (target selection, facing, ...).
    if (moves)
        return 0.0f;

    // Casts are allowed while standing on the safe spot with enough time left before the next eruption.
    uint32 spellId = AI_VALUE2(uint32, "spell id", spellAction->getSpell());
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return 1.0f;

    uint32 castTime = spellInfo->CalcCastTime(bot);
    if (spellInfo->IsChanneled())
    {
        int32 duration = spellInfo->GetDuration();
        if (duration > 0)
            castTime += uint32(duration);
    }
    if (castTime == 0)
        return 1.0f;

    return helper.CanStandStillFor(castTime + 500) ? 1.0f : 0.0f;
}

float LoathebGenericMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "loatheb");
    if (!boss)
        return 1.0f;

    context->GetValue<bool>("neglect threat")->Set(true);
    if (botAI->GetState() == BOT_STATE_COMBAT &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) || dynamic_cast<FleeAction*>(action) ||
         dynamic_cast<CombatFormationMoveAction*>(action)))
    {
        return 0.0f;
    }
    if (!dynamic_cast<CastHealingSpellAction*>(action))
        return 1.0f;

    Aura* aura = NaxxSpellIds::GetAnyAura(bot, {NaxxSpellIds::NecroticAura10});
    if (!aura)
    {
        // Fallback to name for custom spell data.
        aura = botAI->GetAura("necrotic aura", bot);
    }
    if (!aura || aura->GetDuration() <= 1500)
        return 1.0f;

    return 0.0f;
}

float ThaddiusGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (dynamic_cast<CombatFormationMoveAction*>(action))
        return 0.0f;
    // A tank thrown across by Magnetic Pull still has the other platform's pet as target, and generic movement
    // (reach melee) walked it down the ramp towards that pet, dragging its new pet 28+ yd from home: the tesla coil
    // then shocks the raid (raid 10 runs 1634-1637, 32-48 shocks each, the one clean run had none). Until the
    // Thaddius action has switched it to the nearest pet, a tank does not move on its own.
    if (helper.IsPhasePet() && botAI->IsTank(bot) && dynamic_cast<MovementAction*>(action) &&
        !dynamic_cast<ThaddiusAttackNearestPetAction*>(action) && !dynamic_cast<ThaddiusMoveToPlatformAction*>(action))
    {
        // Nor while in the air: at the moment of the throw the old pet is still the nearest, and a taunt spent in
        // flight is on cooldown when the tank lands (1662: both taunts 1 s after the pull, the pets then kept the
        // old tanks, which a pet out of melee reach only drops at 130% threat).
        if (AI_VALUE(Unit*, "current target") != helper.GetNearestPet() ||
            bot->GetPositionZ() > helper.tankPosZ + 5.0f)
            return 0.0f;
    }
    // Magnetic Pull swaps the pets' threat onto the tank each one pulls in, but the tank thrown across still has its
    // old pet as current target and taunted it straight back (raid 10 run 1660: Stalagg's threat on the main tank
    // went 0 -> equal to the DK a second after the pull; the pets then chased the old tanks off their platforms and
    // the coils shocked the raid). A tank only taunts the pet nearest to it.
    if (helper.IsPhasePet() && botAI->IsTank(bot) &&
        (dynamic_cast<CastTauntAction*>(action) || dynamic_cast<CastDarkCommandAction*>(action) ||
         dynamic_cast<CastHandOfReckoningAction*>(action) || dynamic_cast<CastGrowlAction*>(action) ||
         dynamic_cast<CastRighteousDefenseAction*>(action)))
    {
        // Nor while in the air: at the moment of the throw the old pet is still the nearest, and a taunt spent in
        // flight is on cooldown when the tank lands (1662: both taunts 1 s after the pull, the pets then kept the
        // old tanks, which a pet out of melee reach only drops at 130% threat).
        if (AI_VALUE(Unit*, "current target") != helper.GetNearestPet() ||
            bot->GetPositionZ() > helper.tankPosZ + 5.0f)
            return 0.0f;
    }
    // pet phase
    if (helper.IsPhasePet() &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) ||
         dynamic_cast<ReachPartyMemberToHealAction*>(action) || dynamic_cast<BuffOnMainTankAction*>(action)))
    {
        return 0.0f;
    }
    // die at the same time
    Unit* target = AI_VALUE(Unit*, "current target");
    Unit* feugen = AI_VALUE2(Unit*, "find target", "feugen");
    Unit* stalagg = AI_VALUE2(Unit*, "find target", "stalagg");
    // Hold damage on a pet until a tank has it: ranged opened on Feugen before the DK had threat and he killed the
    // balance druid 7 s in (raid 10 run 1558). Healing is unaffected; with no tank left there is nobody to wait for.
    if (helper.IsPhasePet() && !botAI->IsTank(bot) && target && (target == feugen || target == stalagg) &&
        target->GetHealthPct() > 20.0f &&
        ((dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastHealingSpellAction*>(action)) ||
         dynamic_cast<AttackAction*>(action)))
    {
        Unit* victim = target->GetVictim();
        Player* holder = victim ? victim->ToPlayer() : nullptr;
        if (!holder || !PlayerbotAI::IsTank(holder))
        {
            bool tankAlive = false;
            if (Group* group = bot->GetGroup())
                for (GroupReference* ref = group->GetFirstMember(); ref && !tankAlive; ref = ref->next())
                    if (Player* member = ref->GetSource())
                        tankAlive = member->IsAlive() && PlayerbotAI::IsTank(member);
            if (tankAlive)
                return 0.0f;
        }
    }
    // Both have to die within 5 s of each other or they revive (raid 10 run 1641: Stalagg 0% / Feugen 2%, both back
    // to full). Hold the lower one from 40% while the other is 3+ points higher, and from 15% while it is 2+ higher;
    // once both are at 5% or below, finish them.
    Unit* otherPet = target == feugen ? stalagg : (target == stalagg ? feugen : nullptr);
    bool const petsTrailing = target && otherPet && helper.IsPetActive(otherPet) &&
                              !(target->GetHealthPct() <= 5.0f && otherPet->GetHealthPct() <= 5.0f) &&
                              ((target->GetHealthPct() <= 40.0f &&
                                otherPet->GetHealthPct() >= target->GetHealthPct() + 3.0f) ||
                               (target->GetHealthPct() <= 15.0f &&
                                otherPet->GetHealthPct() >= target->GetHealthPct() + 2.0f));
    if (helper.IsPhasePet() && petsTrailing)
    {
        if (dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastHealingSpellAction*>(action))
            return 0.0f;
    }
    // magnetic pull
    // uint32 curr_timer = eventMap->GetTimer();
    // // if (curr_phase == 2 && bot->GetPositionZ() > 312.5f && dynamic_cast<MovementAction*>(action))
    // {
    // if (curr_phase == 2 && (curr_timer % 20000 >= 18000 || curr_timer % 20000 <= 2000) &&
    // dynamic_cast<MovementAction*>(action))
    // {
    //     // MotionMaster *mm = bot->GetMotionMaster();
    //     // mm->Clear();
    //     return 0.0f;
    // }
    // thaddius phase
    // if (curr_phase == 8 && dynamic_cast<FleeAction*>(action))
    // {
    //         return 0.0f;
    // }
    return 1.0f;
}

float SapphironGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (dynamic_cast<CastDeathGripAction*>(action) || dynamic_cast<CombatFormationMoveAction*>(action))
        return 0.0f;

    // Get behind an ice block before casting anything: a bot mid-cast does not move, and the holy paladin kept
    // healing through the breath warning and died to Frost Breath in most air phases (raid 10 runs 1681-1684).
    float hideX = 0.0f, hideY = 0.0f;
    if (helper.IsPhaseFlight() && dynamic_cast<CastSpellAction*>(action) && helper.FindHideSpot(hideX, hideY) &&
        bot->GetExactDist2d(hideX, hideY) > 1.5f)
        return 0.0f;

    return 1.0f;
}

float InstructorRazuviousGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    context->GetValue<bool>("neglect threat")->Set(true);
    if (botAI->GetState() == BOT_STATE_COMBAT &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastTauntAction*>(action) || dynamic_cast<CastDarkCommandAction*>(action) ||
         dynamic_cast<CastHandOfReckoningAction*>(action) || dynamic_cast<CastGrowlAction*>(action)))
    {
        return 0.0f;
    }
    return 1.0f;
}

float KelthuzadGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if ((dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) || dynamic_cast<FleeAction*>(action)))
    {
        return 0.0f;
    }
    if (helper.IsPhaseOne())
    {
        if (dynamic_cast<CastTotemAction*>(action) || dynamic_cast<CastShadowfiendAction*>(action) ||
            dynamic_cast<CastRaiseDeadAction*>(action) || dynamic_cast<CastFeignDeathAction*>(action) ||
            dynamic_cast<CastInvisibilityAction*>(action) || dynamic_cast<CastVanishAction*>(action) ||
            dynamic_cast<PetAttackAction*>(action))
        {
            return 0.0f;
        }
    }
    if (helper.IsPhaseTwo())
    {
        if (dynamic_cast<CastBlizzardAction*>(action) || dynamic_cast<CastFrostNovaAction*>(action))
            return 0.0f;

    }
    return 1.0f;
}

float AnubrekhanGenericMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
        return 1.0f;

    if (NaxxSpellIds::HasAnyAura(
            boss, {NaxxSpellIds::LocustSwarm10, NaxxSpellIds::LocustSwarm10Alt, NaxxSpellIds::LocustSwarm25}) ||
        botAI->HasAura("locust swarm", boss))
    {
        if (dynamic_cast<FleeAction*>(action))
            return 0.0f;
    }
    return 1.0f;
}

// "maexxna attack web wrap" switches the target to a cocoon, then has nothing to do while the target already is the
// cocoon, and the next action down the queue is "dps assist", which picks Maexxna again. The cocoon only took damage
// between two switches; a bot stayed wrapped for 58 s. Hold dps assist while the current target is a live cocoon.
float MaexxnaGenericMultiplier::GetValue(Action* action)
{
    if (!dynamic_cast<DpsAssistAction*>(action))
        return 1.0f;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (target && target->IsAlive() && target->GetEntry() == NaxxSpellIds::NpcWebWrap)
        return 0.0f;

    return 1.0f;
}

float FourHorsemenGenericMultiplier::GetValue(Action* action)
{
    // By entry and only in combat: the threat-list lookup left dps/tank assist on for every bot without threat on
    // Zeliek, and they wandered off the kill order.
    if (!bot->IsInCombat() || !bot->FindNearestCreature(16063, 200.0f, true))
        return 1.0f;

    context->GetValue<bool>("neglect threat")->Set(true);
    if ((dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action)))
        return 0.0f;

    // An attractor off its post leaves Zeliek or the Lady with nobody inside 45 yd and they punish the whole raid:
    // run 1734 the Zeliek attractor (holy paladin) ran to the dying main tank from 21 to 30 s and Condemnation hit
    // all ten twice. Attractors only move for their post or to step out of a void zone; they heal from where they stand.
    if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<FourHorsemenAttractAlternativelyAction*>(action) &&
        !dynamic_cast<AvoidAoeAction*>(action) && FourHorsemenBossHelper(botAI).IsAttracter(bot))
        return 0.0f;

    return 1.0f;
}

// float GothikGenericMultiplier::GetValue(Action* action)
// {
//     Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
//     if (!boss)
//     {
//         return 1.0f;
//     }
//     BossAI* boss_ai = dynamic_cast<BossAI*>(boss->GetAI());
//     EventMap* eventMap = boss_botAI->GetEvents();
//     uint32 curr_phase = eventMap->GetPhaseMask();
//     if (curr_phase == 1 && (dynamic_cast<FollowAction*>(action)))
//     {
//         return 0.0f;
//     }
//     if (curr_phase == 1 && (dynamic_cast<AttackAction*>(action)))
//     {
//         Unit* target = action->GetTarget();
//         if (target == boss)
//         {
//             return 0.0f;
//         }
//     }
//     return 1.0f;
// }

float GluthGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if ((dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<FleeAction*>(action) || dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) ||
         dynamic_cast<CastStarfallAction*>(action)))
    {
        return 0.0f;
    }

    // A tank carrying 5+ Mortal Wound stacks does not take Gluth back (Righteous Defense is a taunt too).
    if (botAI->IsTank(bot) && GluthBossHelper::MortalWoundStacks(bot) >= 5)
    {
        if (dynamic_cast<CastTauntAction*>(action) || dynamic_cast<CastDarkCommandAction*>(action) ||
            dynamic_cast<CastHandOfReckoningAction*>(action) || dynamic_cast<CastGrowlAction*>(action) ||
            dynamic_cast<CastRighteousDefenseAction*>(action))
        {
            return 0.0f;
        }
    }
    if (dynamic_cast<PetAttackAction*>(action))
    {
        Unit* target = AI_VALUE(Unit*, "current target");
        if (helper.IsZombieChow(target))
            return 0.0f;
    }
    return 1.0f;
}
