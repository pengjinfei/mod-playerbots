/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "OCActions.h"
#include "InstanceScript.h"
#include "LastSpellCastValue.h"
#include "OCTriggers.h"
#include "Playerbots.h"

bool AvoidUnstableSphereAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "drakos the interrogator");
    if (!boss) { return false; }

    float radius = 12.0f;
    float extraDistance = 1.0f;
    Unit* closestSphere = nullptr;

    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (unit && unit->GetEntry() == NPC_UNSTABLE_SPHERE && !unit->isMoving())
        {
            if (!closestSphere || bot->GetExactDist2d(unit) < bot->GetExactDist2d(closestSphere))
            {
                closestSphere = unit;
            }
        }
    }

    if (closestSphere && bot->GetExactDist2d(closestSphere) < radius + extraDistance)
    {
        return MoveAway(closestSphere, fmin(3.0f, bot->GetExactDist2d(closestSphere) - radius + extraDistance));
    }

    return false;
}

bool MountDrakeAction::isPossible() { return bot->GetMapId() == OCULUS_MAP_ID; }
bool MountDrakeAction::Execute(Event /*event*/)
{
    std::map<int32, int32> drakeAssignments;
    // Composition can be adjusted - both 3/1/1 and 2/2/1 are good default comps
    // {Amber, Emerald, Ruby}
    std::vector<uint8> composition = {2, 2, 1};
    // std::vector<uint8> composition = {3, 1, 1};
    int32 myIndex = botAI->GetGroupSlotIndex(bot);

    // With a master, subtract the player's chosen mount type from the composition so the player can play whichever
    // they prefer. A masterless party runs 3 Amber / 2 Emerald: the Ruby drake builds Evasive Charges only while
    // it is attacked, and Eregos went for the healing Emerald instead, so Martyr came up once in a whole fight
    // (heroic run1312) while the Ruby's Searing Wrath added little damage.
    if (!botAI->GetMaster())
        composition = {3, 2, 0};
    if (Player* master = botAI->GetMaster())
    {
        Unit* vehicle = master->GetVehicleBase();
        if (!vehicle) { return false; }

        switch (vehicle->GetEntry())
        {
            case NPC_AMBER_DRAKE:
                composition[0]--;
                break;
            case NPC_EMERALD_DRAKE:
                composition[1]--;
                break;
            case NPC_RUBY_DRAKE:
                composition[2]--;
                break;
        }
    }

    std::vector<Player*> players = botAI->GetAllPlayersInGroup();
    for (Player* player : players)
    {
        if (!player || !player->IsInWorld() || player->IsDuringRemoveFromWorld())
            continue;

        WorldSession* session = player->GetSession();
        if (!session || !session->IsBot())
            continue;

        int slot = botAI->GetGroupSlotIndex(player);
        if (slot < 0)
            continue;

        for (uint8 i = 0; i < composition.size(); ++i)
        {
            if (composition[i] > 0)
            {
                drakeAssignments[slot] = DRAKE_ITEMS[i];
                composition[i]--;
                break;
            }
        }
    }

    // Correct/update the drake items in inventories incase assignments have changed
    for (int64_t itemId : DRAKE_ITEMS)
    {
        Item* item = bot->GetItemByEntry(itemId);
        if (!item) { continue; }

        if (itemId == drakeAssignments[myIndex])
        {
            // Use our assigned drake
            return UseItemAuto(item);
        }
        // Else assigned drake is different, destroy old drake
        uint32 count = 1;
        bot->DestroyItemCount(item, count, true);
        break;
    }

    // Bot does not have the correct drake item. The drake NPCs only hand out essences once Drakos is dead (their
    // gossip checks DATA_DRAKOS), so do not grant one before that either.
    InstanceScript* instance = bot->GetInstanceScript();
    if (!instance || instance->GetData(0 /*DATA_DRAKOS*/) != DONE)
        return false;

    bot->AddItem(drakeAssignments[myIndex], 1);
    return false;
}

bool DismountDrakeAction::Execute(Event /*event*/)
{
    if (bot->GetVehicle())
    {
        bot->ExitVehicle();
        return true;
    }
    return false;
}

bool OccFlyDrakeAction::Execute(Event /*event*/)
{
    Unit* vehicleBase = bot->GetVehicleBase();
    if (!vehicleBase) { return false; }

    // Channeled drake spells (Dream Funnel, Temporal Rift) cannot start while the drake moves and stop when it
    // does (PlayerbotAI::CanCastVehicleSpell / CastVehicleSpell): do not move a channeling drake.
    if (vehicleBase->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        return false;

    MotionMaster* mm = vehicleBase->GetMotionMaster();
    Unit* boss = AI_VALUE2(Unit*, "find target", "ley-guardian eregos");
    if (!boss && !botAI->GetMaster())
        boss = OccMasterlessDrakeTarget(bot);
    if (boss && !boss->HasAura(SPELL_PLANAR_SHIFT))
    {
        // Handle as boss encounter instead of formation flight. Drake spells reach 60 yd; Eregos wanders 50 yd,
        // so close to 45 yd to keep him in range without chasing him every tick.
        float distance = vehicleBase->GetExactDist(boss);
        float range = 45.0f;
        if (distance > range + 10.0f)
        {
            // MoveForwards(target, dist) ends dist yards from the target on our side; the old "range - distance" was
            // negative here and put the end point on the far side of Eregos.
            mm->Clear(false);
            vehicleBase->SetCanFly(true);
            mm->MoveForwards(boss, range);
            vehicleBase->SendMovementFlagUpdate();
            return true;
        }

        vehicleBase->SetFacingToObject(boss);
        mm->MoveIdle();
        vehicleBase->SendMovementFlagUpdate();
        return false;
    }

    Player* master = botAI->GetMaster();
    Unit* masterVehicle = master ? master->GetVehicleBase() : nullptr;
    if (!masterVehicle) { return false; }

    if (vehicleBase->GetExactDist(masterVehicle) > 20.0f)
    {
        // 3/4 of a circle, with frontal cone 90 deg unobstructed
        float angle = botAI->GetGroupSlotIndex(bot) * (2*M_PI - M_PI_2)/5 + M_PI_2;
        vehicleBase->SetCanFly(true);
        mm->MoveFollow(masterVehicle, 15.0f, angle);
        vehicleBase->SendMovementFlagUpdate();
        return true;
    }
    return false;
}

bool AvoidPlanarAnomalyAction::Execute(Event /*event*/)
{
    Unit* vehicleBase = bot->GetVehicleBase();
    if (!vehicleBase)
        return false;

    // Fly directly away from the anomalies in range; they chase at run speed, a drake outruns them.
    std::list<Creature*> anomalies;
    vehicleBase->GetCreatureListWithEntryInGrid(anomalies, NPC_PLANAR_ANOMALY, PLANAR_ANOMALY_DANGER_RANGE);
    float dx = 0.0f;
    float dy = 0.0f;
    for (Creature* anomaly : anomalies)
    {
        if (!anomaly->IsAlive())
            continue;

        float const dist = std::max(1.0f, vehicleBase->GetExactDist2d(anomaly));
        dx += (vehicleBase->GetPositionX() - anomaly->GetPositionX()) / dist;
        dy += (vehicleBase->GetPositionY() - anomaly->GetPositionY()) / dist;
    }

    float angle = (dx == 0.0f && dy == 0.0f) ? vehicleBase->GetOrientation() : std::atan2(dy, dx);
    // Stay near the fight: with Eregos far behind, bend the escape sideways around him instead of away.
    constexpr float fleeDistance = 40.0f;
    constexpr float maxBossDistance = 80.0f;
    Unit* boss = AI_VALUE2(Unit*, "find target", "ley-guardian eregos");
    float x = vehicleBase->GetPositionX() + fleeDistance * std::cos(angle);
    float y = vehicleBase->GetPositionY() + fleeDistance * std::sin(angle);
    if (boss && boss->GetExactDist2d(x, y) > maxBossDistance)
    {
        float const toBoss = vehicleBase->GetAbsoluteAngle(boss);
        float const side = Position::NormalizeOrientation(angle - toBoss) < M_PI ? M_PI_2 : -M_PI_2;
        angle = toBoss + side;
        x = vehicleBase->GetPositionX() + fleeDistance * std::cos(angle);
        y = vehicleBase->GetPositionY() + fleeDistance * std::sin(angle);
    }

    if (vehicleBase->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        vehicleBase->InterruptSpell(CURRENT_CHANNELED_SPELL);

    MotionMaster* mm = vehicleBase->GetMotionMaster();
    mm->Clear(false);
    vehicleBase->SetCanFly(true);
    mm->MovePoint(0, x, y, vehicleBase->GetPositionZ(), FORCED_MOVEMENT_NONE, 0.0f, 0.0f, false, true);
    vehicleBase->SendMovementFlagUpdate();
    if (!sPlayerbotAIConfig.logInGroupOnly)
        LOG_DEBUG("playerbots", "eregos-anomaly bot={} anomalies={} flee=({:.1f},{:.1f})", bot->GetName(),
                  anomalies.size(), x, y);
    return true;
}

bool OccDrakeAttackAction::Execute(Event /*event*/)
{
    vehicleBase = bot->GetVehicleBase();
    if (!vehicleBase) { return false; }

    Unit* target = AI_VALUE(Unit*, "current target");

    if (!target)
    {
        GuidVector npcs = AI_VALUE(GuidVector, "possible targets");
        for (auto& npc : npcs)
        {
            Unit* unit = botAI->GetUnit(npc);
            if (!unit || !unit->IsInCombat()) { continue; }

            target = unit;
            break;
        }
    }
    if (!target && !botAI->GetMaster())
        target = OccMasterlessDrakeTarget(bot);
    // Check this again to see if a target was assigned
    if (!target) { return false; }

    switch (vehicleBase->GetEntry())
    {
        case NPC_AMBER_DRAKE:
            return AmberDrakeAction(target);
        case NPC_EMERALD_DRAKE:
            return EmeraldDrakeAction(target);
        case NPC_RUBY_DRAKE:
            return RubyDrakeAction(target);
        default:
            break;
    }
    return false;
}

bool OccDrakeAttackAction::CastDrakeSpellAction(Unit* target, uint32 spellId, uint32 cooldown)
{
    if (botAI->CanCastVehicleSpell(spellId, target))
        if (botAI->CastVehicleSpell(spellId, target))
        {
            vehicleBase->AddSpellCooldown(spellId, 0, cooldown);
            if (!sPlayerbotAIConfig.logInGroupOnly)
                LOG_DEBUG("playerbots", "drake-cast bot={} drake={} spell={} target={}", bot->GetName(),
                          vehicleBase->GetEntry(), spellId, target->GetName());
            return true;
        }
    return false;
}

bool OccDrakeAttackAction::AmberDrakeAction(Unit* target)
{
    Aura* shockCharges = target->GetAura(SPELL_SHOCK_CHARGE, vehicleBase->GetGUID());
    if (shockCharges && shockCharges->GetStackAmount() > 8)
    {
        // At 9 charges, better to detonate and re-channel rather than stacking the last charge due to gcd
        // If stacking Amber drakes, may need to drop this even lower as the charges stack so fast
        if (CastDrakeSpellAction(target, SPELL_SHOCK_LANCE, 0))
            return true;
    }

    // Deal with enrage after shock charges, as Stop Time adds 5 charges and they may get wasted
    if (target->HasAura(SPELL_ENRAGED_ASSAULT) &&
        !target->HasAura(SPELL_STOP_TIME) &&
        !vehicleBase->HasSpellCooldown(SPELL_STOP_TIME))
    {
        if (CastDrakeSpellAction(target, SPELL_STOP_TIME, 60000))
            return true;
    }

    if (!vehicleBase->FindCurrentSpellBySpellId(SPELL_TEMPORAL_RIFT))
    {
        // Channeled: a moving drake cannot start it. Stop and try in the same tick.
        if (vehicleBase->isMoving())
            vehicleBase->StopMoving();
        if (CastDrakeSpellAction(target, SPELL_TEMPORAL_RIFT, 0))
            return true;
    }

    return false;
}

bool OccDrakeAttackAction::EmeraldDrakeAction(Unit* target)
{
    // Lowest-health other drake in the group. Dream Funnel is the group's only healing on drakes; in masterless
    // run1282 it was never cast and three drakes died within 50 s.
    Unit* healingTarget = nullptr;
    GuidVector members = AI_VALUE(GuidVector, "group members");
    for (auto& member : members)
    {
        Unit* unit = botAI->GetUnit(member);
        if (!unit || bot->GetGUID() == member)
        {
            continue;
        }

        Unit* drake = unit->GetVehicleBase();
        if (!drake || !drake->IsAlive() || drake->IsFullHealth()) { continue; }

        if (!healingTarget || drake->GetHealthPct() < healingTarget->GetHealthPct())
        {
            healingTarget = drake;
        }
    }

    Spell* currentSpell = vehicleBase->FindCurrentSpellBySpellId(SPELL_DREAM_FUNNEL);
    auto funnel = [&]() -> bool
    {
        if (!healingTarget || (currentSpell && currentSpell->m_targets.GetUnitTarget() == healingTarget))
            return false;

        float distance = vehicleBase->GetExactDist(healingTarget);
        float range = 58.0f;  // Dream Funnel reaches 60 yd
        if (distance > range)
        {
            // MoveForwards(target, dist) ends dist yards short of the target on our side.
            MotionMaster* mm = vehicleBase->GetMotionMaster();
            mm->Clear(false);
            mm->MoveForwards(healingTarget, range - 10.0f);
            vehicleBase->SendMovementFlagUpdate();
            return true;
        }
        // A moving drake cannot start the channel. Stop and try in the same tick: returning here and trying next
        // tick left the drake stopping and being moved again every tick (run1310: 30 attempts, no Dream Funnel).
        if (vehicleBase->isMoving())
            vehicleBase->StopMoving();
        return CastDrakeSpellAction(healingTarget, SPELL_DREAM_FUNNEL, 0);
    };

    // Heal before damage whenever a drake is hurt: Leeching Poison refreshes took every global cooldown before.
    if (healingTarget && healingTarget->HealthBelowPct(90) && funnel())
        return true;

    Aura* poisonStacks = target->GetAura(SPELL_LEECHING_POISON, vehicleBase->GetGUID());
    if (!poisonStacks || (poisonStacks->GetStackAmount() < 3 ||
                         poisonStacks->GetDuration() < 4000))
    {
        if (CastDrakeSpellAction(target, SPELL_LEECHING_POISON, 0))
            return true;
    }

    if (!vehicleBase->HasSpellCooldown(SPELL_TOUCH_THE_NIGHTMARE) &&
        (!target->HasAura(SPELL_TOUCH_THE_NIGHTMARE) || vehicleBase->HealthAbovePct(90)))
    {
        if (CastDrakeSpellAction(target, SPELL_TOUCH_THE_NIGHTMARE, 10000))
            return true;
    }

    if (funnel())
        return true;

    // Fill GCDs with Leeching Poison to refresh timer, rather than idling
    if (!currentSpell)
    {
        if (CastDrakeSpellAction(target, SPELL_LEECHING_POISON, 0))
            return true;
    }

    return false;
}

bool OccDrakeAttackAction::RubyDrakeAction(Unit* target)
{
    Aura* evasiveCharges = vehicleBase->GetAura(SPELL_EVASIVE_CHARGES);
    Aura* evasiveManeuvers = vehicleBase->GetAura(SPELL_EVASIVE_MANEUVERS);

    if (evasiveCharges)
    {
        if (evasiveManeuvers &&
            !vehicleBase->HasSpellCooldown(SPELL_MARTYR) &&
            evasiveManeuvers->GetDuration() > 10000 &&
            evasiveCharges->GetStackAmount() >= 5)
        {
            if (CastDrakeSpellAction(vehicleBase, SPELL_MARTYR, 10000))
                return true;
        }

        if (!vehicleBase->HasSpellCooldown(SPELL_EVASIVE_MANEUVERS) &&
            evasiveCharges->GetStackAmount() >= 10)
        {
            if (CastDrakeSpellAction(vehicleBase, SPELL_EVASIVE_MANEUVERS, 5000))
                return true;
        }
    }

    if (CastDrakeSpellAction(target, SPELL_SEARING_WRATH, 0))
            return true;
}

bool AvoidArcaneExplosionAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "mage-lord urom");
    if (!boss) { return false; }

    Position const* closestPos = nullptr;

    for (auto& position : uromSafePositions)
    {
        if (!closestPos || bot->GetExactDist(position) < bot->GetExactDist(closestPos))
            {
                closestPos = &position;
            }
    }

    if (!closestPos) { return false; }

    return MoveNear(bot->GetMapId(), closestPos->GetPositionX(), closestPos->GetPositionY(), closestPos->GetPositionZ(), 2.0f, MovementPriority::MOVEMENT_COMBAT);
}

bool TimeBombSpreadAction::Execute(Event /*event*/)
{
    float radius = 10.0f;
    float distanceExtra = 2.0f;

    GuidVector members = AI_VALUE(GuidVector, "group members");
    for (auto& member : members)
    {
        if (bot->GetGUID() == member)
        {
            continue;
        }

        Unit* unit = botAI->GetUnit(member);
        if (unit && bot->GetExactDist2d(unit) < radius)
        {
            return MoveAway(unit, radius + distanceExtra - bot->GetExactDist2d(unit));
        }
    }
    return false;
}
