/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NaxxActions.h"
#include "Playerbots.h"

bool FourHorsemenAttractAlternativelyAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    helper.CalculatePosToGo(bot);
    auto [posX, posY] = helper.CurrentAttractPos();
    // The Lady drops her Void Zone (16697) on her target, which is the attractor on its post; walking back to the
    // exact post put the hunter in it again every time avoid aoe stepped it out (run 1751: 43 ticks, 34k, dead at
    // 81 s). When a zone covers the post, hold the nearest point around it that is clear of every zone, inside
    // 42 yd of this post's horseman and outside 47 yd of the other one: a plain step away from the zone put the
    // Lady post 40.7 yd from Zeliek and both attractors kept stacking his mark through the swap (run 1755, 6 stacks).
    std::list<Creature*> zones;
    bot->GetCreatureListWithEntryInGrid(zones, 16697, 60.0f);
    zones.remove_if([](Creature* zone) { return !zone || !zone->IsAlive(); });
    auto clearOfZones = [&](float x, float y)
    {
        for (Creature* zone : zones)
            if (zone->GetExactDist2d(x, y) < 7.0f)
                return false;
        return true;
    };
    if (!clearOfZones(posX, posY))
    {
        Unit* own = helper.CurrentAttackTarget();
        Unit* other = own == helper.Zeliek() ? helper.Lady() : helper.Zeliek();
        float bestX = posX, bestY = posY, bestDist = -1.0f;
        for (float radius : {5.0f, 8.0f, 11.0f})
            for (int i = 0; i < 12; ++i)
            {
                float const angle = float(M_PI) * 2.0f * i / 12.0f;
                float const x = posX + radius * std::cos(angle), y = posY + radius * std::sin(angle);
                if (!clearOfZones(x, y))
                    continue;
                if (own && own->GetExactDist2d(x, y) > 42.0f)
                    continue;
                if (other && other->GetExactDist2d(x, y) < 47.0f)
                    continue;
                if (bestDist < 0.0f || radius < bestDist)
                {
                    bestX = x;
                    bestY = y;
                    bestDist = radius;
                }
            }
        posX = bestX;
        posY = bestY;
    }
    if (bot->GetExactDist2d(posX, posY) > 1.5f && MoveTo(bot->GetMapId(), posX, posY, helper.posZ, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
        return true;

    Unit* attackTarget = helper.CurrentAttackTarget();
    if (attackTarget && context->GetValue<Unit*>("current target")->Get() != attackTarget)
        return Attack(attackTarget);

    return false;
}

bool FourHorsemenAttackInOrderAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    // By entry: "find target" only sees units already on the bot's threat list, so the next horseman in the order
    // was invisible until someone else pulled it onto this bot.
    Unit* target = nullptr;
    Unit* thane = bot->FindNearestCreature(16064, 200.0f, true);
    Unit* lady = bot->FindNearestCreature(16065, 200.0f, true);
    Unit* sir = bot->FindNearestCreature(16063, 200.0f, true);
    Unit* fourth = bot->FindNearestCreature(30549, 200.0f, true);
    if (!fourth)
        fourth = bot->FindNearestCreature(16062, 200.0f, true);

    std::vector<Unit*> attack_order;
    if (botAI->IsAssistTank(bot))
        attack_order = {fourth, thane, lady, sir};
    else
        attack_order = {thane, fourth, lady, sir};
    for (Unit* t : attack_order)
    {
        if (t && t->IsAlive())
        {
            target = t;
            break;
        }
    }
    if (target)
    {
        // Walk over first when the next horseman is across the room: from the Baron's corner the Lady is 107 yd
        // away and the raid stood idle for 80 s after the Baron died (runs 1757/1759/1762). This goes before the
        // "already my target" exit: the bots had already switched to her and returned there, and the regular
        // chase is held by the healer leash.
        if (!bot->IsWithinLOSInMap(target) || bot->GetExactDist2d(target) > 35.0f)
            return MoveNear(target, 22.0f, MovementPriority::MOVEMENT_COMBAT);

        if (context->GetValue<Unit*>("current target")->Get() == target && botAI->GetState() == BOT_STATE_COMBAT)
            return false;

        return Attack(target);
    }
    return false;
}

namespace
{
constexpr uint32 NPC_THANE_KORTHAZZ = 16064;
constexpr uint32 NPC_BARON_RIVENDARE = 30549;
constexpr uint32 SPELL_MARK_OF_RIVENDARE = 28834;
// Rivendare's final waypoint (boss_four_horsemen WaypointPositions[8]).
constexpr float BARON_CORNER_X = 2583.9f, BARON_CORNER_Y = -2971.6f, BARON_CORNER_Z = 241.35f;
// The main tank holds Thane 5 yd deeper into his corner than the waypoint, 65 yd from the Baron's corner: tanked at
// (2542.6,-2999.7) he was 47 yd from the Baron, took Baron marks and had to wait them off before he could take the
// Baron (run 1733: swap 25 s late, off tank dead to the 5th/6th mark). The ranged stand 5 yd past him, about 70 yd
// from the Baron; the relieved tank waits there too. Flat floor, Thane in sight. Front marks reach 55 yd (Spell.dbc
// radius index 35) and last 25 s.
constexpr float THANE_SPOT_X = 2539.5f, THANE_SPOT_Y = -3018.6f, THANE_SPOT_Z = 241.35f;
constexpr float RAID_SPOT_X = 2536.0f, RAID_SPOT_Y = -3022.3f, RAID_SPOT_Z = 241.35f;
constexpr float MARK_CLEAR_DISTANCE = 58.0f;
// Off-tank healer while Thane lives: 35 yd from the Baron's corner toward Thane, 30 yd from the Thane spot, so one
// healer reaches both tanks (flat floor, both in sight). A blood DK alone on the Baron lost 46k in 21 s and died
// 10 s before Thane (run 1742).
constexpr float OFFTANK_HEAL_SPOT_X = 2559.9f, OFFTANK_HEAL_SPOT_Y = -2997.0f, OFFTANK_HEAL_SPOT_Z = 241.4f;

uint8 RivendareMarks(Unit* unit)
{
    Aura* aura = unit ? unit->GetAura(SPELL_MARK_OF_RIVENDARE) : nullptr;
    return aura ? aura->GetStackAmount() : 0;
}

// The living non-attractor healer with the lowest GUID heals the off tank; the others stay with the raid. Group
// order changes between attempts (1745 the paladin, 1753 the priest), the GUID does not, so the raid leader can
// place that healer before the pull.
bool IsOffTankHealer(PlayerbotAI* botAI, Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group || !PlayerbotAI::IsHeal(bot))
        return false;
    FourHorsemenBossHelper helper(botAI);
    Player* pick = nullptr;
    uint32 healers = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive() || !PlayerbotAI::IsHeal(member) || helper.IsAttracter(member))
            continue;
        if (!pick || member->GetGUID().GetCounter() < pick->GetGUID().GetCounter())
            pick = member;
        ++healers;
    }
    return healers >= 2 && pick == bot;
}
}

// 10-man front side with two tanks. While Thane lives, the off tank holds the Baron in his own corner and the ranged
// and healers stand past Thane, out of the Baron's mark (run 1729: the Baron followed the main tank to Thane, the
// whole raid stacked both front marks and the fifth Baron mark, 12.5k each, started the wipe at 72 s). Once Thane is
// down, the tanks trade the Baron by mark stacks: the tank without marks taunts when the holder reaches three, the
// relieved tank waits outside 55 yd until its marks drop.
bool FourHorsemenFrontAction::Execute(Event event)
{
    Creature* baron = bot->FindNearestCreature(NPC_BARON_RIVENDARE, 200.0f, true);
    if (!baron)
    {
        // Back two: healers that are not attractors join the raid on the current kill target. The off-tank healer
        // otherwise stayed in the Baron's corner, 109 yd from the Lady, and the melee on her died unhealed (run 1745).
        if (!PlayerbotAI::IsHeal(bot) || bot->FindNearestCreature(NPC_THANE_KORTHAZZ, 200.0f, true))
            return false;
        Unit* target = bot->FindNearestCreature(16065, 200.0f, true);
        if (!target)
            target = bot->FindNearestCreature(16063, 200.0f, true);
        if (!target || bot->GetExactDist2d(target) <= 25.0f)
            return false;
        MoveNear(target, 20.0f, MovementPriority::MOVEMENT_COMBAT);
        return true;
    }
    Creature* thane = bot->FindNearestCreature(NPC_THANE_KORTHAZZ, 200.0f, true);
    bool const tank = botAI->IsTank(bot);

    auto tauntBaron = [&]() -> bool
    {
        if (bot->GetDistance(baron) > 25.0f)
            return MoveNear(baron, 20.0f, MovementPriority::MOVEMENT_COMBAT) || true;
        if (AI_VALUE(Unit*, "current target") != baron)
            return Attack(baron);
        return botAI->DoSpecificAction("taunt spell", event, true);
    };
    auto stayOutOfMarks = [&]() -> bool
    {
        float const dist = bot->GetExactDist2d(baron);
        if (dist >= MARK_CLEAR_DISTANCE)
            return true;  // hold here, nothing to chase
        // Never straight away from the Baron: from his corner that runs into the death knight packs (runs 1733 and
        // 1761 pulled them). Go to whichever known-safe spot inside the room is farthest from him.
        struct SafeSpot { float x, y, z; };
        static SafeSpot const spots[] = {
            {RAID_SPOT_X, RAID_SPOT_Y, RAID_SPOT_Z},  // past Thane's corner
            {2506.9f, -2958.8f, 243.3f},             // platform steps
            {2494.0f, -2962.0f, 241.3f},             // west floor (raid prep point)
        };
        SafeSpot const* best = &spots[0];
        for (SafeSpot const& spot : spots)
            if (baron->GetExactDist2d(spot.x, spot.y) > baron->GetExactDist2d(best->x, best->y))
                best = &spot;
        MoveTo(bot->GetMapId(), best->x, best->y, best->z, false, false, false, false,
               MovementPriority::MOVEMENT_COMBAT);
        return true;
    };

    if (thane)
    {
        if (botAI->IsAssistTank(bot))
        {
            // While the Baron still walks to his corner (passive, no victim) wait for him there: running out to meet
            // him took the off tank away from his healer and he died at 19 s (run 1753).
            if (!baron->GetVictim())
            {
                if (bot->GetExactDist2d(BARON_CORNER_X, BARON_CORNER_Y) > 3.0f)
                    MoveTo(bot->GetMapId(), BARON_CORNER_X, BARON_CORNER_Y, BARON_CORNER_Z, false, false, false, false,
                           MovementPriority::MOVEMENT_COMBAT);
                return true;
            }
            if (baron->GetVictim() != bot)
                return tauntBaron();
            if (baron->GetExactDist2d(BARON_CORNER_X, BARON_CORNER_Y) > 8.0f &&
                bot->GetExactDist2d(BARON_CORNER_X, BARON_CORNER_Y) > 3.0f)
                return MoveTo(bot->GetMapId(), BARON_CORNER_X, BARON_CORNER_Y, BARON_CORNER_Z, false, false, false, false,
                              MovementPriority::MOVEMENT_COMBAT);
            return false;
        }
        if (tank)
        {
            if (thane->GetVictim() == bot && thane->GetExactDist2d(THANE_SPOT_X, THANE_SPOT_Y) > 4.0f &&
                bot->GetExactDist2d(THANE_SPOT_X, THANE_SPOT_Y) > 2.0f)
                return MoveTo(bot->GetMapId(), THANE_SPOT_X, THANE_SPOT_Y, THANE_SPOT_Z, false, false, false, false,
                              MovementPriority::MOVEMENT_COMBAT);
            return false;
        }
        if (IsOffTankHealer(botAI, bot))
        {
            if (bot->GetExactDist2d(OFFTANK_HEAL_SPOT_X, OFFTANK_HEAL_SPOT_Y) <= 2.0f)
                return false;
            MoveTo(bot->GetMapId(), OFFTANK_HEAL_SPOT_X, OFFTANK_HEAL_SPOT_Y, OFFTANK_HEAL_SPOT_Z, false, false, false,
                   false, MovementPriority::MOVEMENT_COMBAT);
            return true;
        }
        if (!botAI->IsRanged(bot))
            return false;
        if (bot->GetExactDist2d(baron) >= 60.0f && bot->GetExactDist2d(RAID_SPOT_X, RAID_SPOT_Y) <= 12.0f)
            return false;
        if (bot->GetExactDist2d(RAID_SPOT_X, RAID_SPOT_Y) > 2.0f)
            MoveTo(bot->GetMapId(), RAID_SPOT_X, RAID_SPOT_Y, RAID_SPOT_Z, false, false, false, false,
                   MovementPriority::MOVEMENT_COMBAT);
        return bot->GetExactDist2d(RAID_SPOT_X, RAID_SPOT_Y) > 2.0f;
    }

    // Everyone else follows the tanks' rule: at four marks leave before the fifth (12.5k) and come back once the
    // aura has dropped (it expires whole, 25 s after the last mark).
    if (!tank)
        return RivendareMarks(bot) >= 4 ? stayOutOfMarks() : false;

    Unit* victim = baron->GetVictim();
    if (victim == bot)
        return false;
    Player* holder = victim ? victim->ToPlayer() : nullptr;
    uint8 const mine = RivendareMarks(bot);
    if (!holder || !botAI->IsTank(holder))
        return mine < 4 ? tauntBaron() : stayOutOfMarks();
    // One stack does no damage; waiting it off would leave the holder on the Baron for another 25 s.
    if (mine <= 1 && RivendareMarks(holder) >= 3)
        return tauntBaron();
    return stayOutOfMarks();
}
