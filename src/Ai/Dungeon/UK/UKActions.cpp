/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UKActions.h"
#include "Creature.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Playerbots.h"
#include "SpellAuraEffects.h"
#include "StringFormat.h"

#include <algorithm>
#include <cmath>
#include <list>

namespace
{
std::string DescribeIngvarControlAuras(Player const* bot)
{
    std::string controlAuras;
    auto appendAuras = [bot, &controlAuras](AuraType type, char const* label)
    {
        for (AuraEffect const* aura : bot->GetAuraEffectsByType(type))
        {
            if (!aura)
                continue;

            if (!controlAuras.empty())
                controlAuras += ',';

            controlAuras += Acore::StringFormat("{}:{}", label, aura->GetId());
        }
    };

    appendAuras(SPELL_AURA_MOD_ROOT, "root");
    appendAuras(SPELL_AURA_MOD_STUN, "stun");
    appendAuras(SPELL_AURA_MOD_FEAR, "fear");
    appendAuras(SPELL_AURA_MOD_CONFUSE, "confuse");
    appendAuras(SPELL_AURA_MOD_CHARM, "charm");
    appendAuras(SPELL_AURA_AOE_CHARM, "aoe_charm");
    appendAuras(SPELL_AURA_MOD_POSSESS, "possess");
    appendAuras(SPELL_AURA_MOD_POSSESS_PET, "possess_pet");
    return controlAuras.empty() ? "none" : controlAuras;
}

std::string DescribeIngvarMovementState(PlayerbotAI* botAI, Player* bot)
{
    MotionMaster* motion = bot->GetMotionMaster();
    MovementGeneratorType const currentMotion = motion ? motion->GetCurrentMovementGeneratorType() : NULL_MOTION_TYPE;
    MovementGeneratorType const controlledMotion = motion ? motion->GetMotionSlotType(MOTION_SLOT_CONTROLLED) : NULL_MOTION_TYPE;
    return Acore::StringFormat(
        "lost_control={} rooted={} frozen={} charmed={} polymorphed={} dead={} ghost={} spirit_redemption={} control_auras={} "
        "controlled_motion={} current_motion={} flight={} teleporting={} vehicle={} vehicle_controllable={}",
        bot->HasUnitState(UNIT_STATE_LOST_CONTROL), bot->IsRooted(), bot->isFrozen(), bot->IsCharmed(),
        bot->IsPolymorphed(), bot->isDead(), bot->HasPlayerFlag(PLAYER_FLAGS_GHOST),
        bot->HasSpiritOfRedemptionAura(), DescribeIngvarControlAuras(bot), uint32(controlledMotion), uint32(currentMotion), bot->IsInFlight(),
        bot->IsBeingTeleported(), bot->GetVehicle() != nullptr, botAI->IsInVehicle(true));
}
}

bool AttackFrostTombAction::isUseful() { return !botAI->IsHeal(bot); }
bool AttackFrostTombAction::Execute(Event /*event*/)
{
    Unit* frostTomb = nullptr;

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->GetEntry() == NPC_FROST_TOMB)
        {
            frostTomb = unit;
            break;
        }
    }
    if (!frostTomb || AI_VALUE(Unit*, "current target") == frostTomb)
    {
        return false;
    }
    return Attack(frostTomb);
}

// TODO: Possibly add player stacking behaviour close to tank, to prevent Skarvald charging ranged
bool AttackDalronnAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "dalronn the controller");
    if (!boss) { return false; }

    if (AI_VALUE(Unit*, "current target") == boss)
    {
        return false;
    }
    return Attack(boss);
}

bool IngvarDodgeSmashAction::isUseful()
{
    constexpr float kIngvarSmashConeRadians = 1.04719755f;
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss)
        return false;

    // `behind` was the wrong criterion. `WorldObjectSpellConeTargetCheck` selects on
    // `IsWithinBoundaryRadius(target) || isInFront(target, coneAngle)`, and the boundary
    // radius (2.0 yd for a player) skips the angle check entirely, so a tank standing in
    // contact behind Ingvar is still picked by effect 0. Ask the two real conditions.
    float const distance = bot->GetExactDist2d(boss);
    bool const insideBypass = distance < kIngvarMeleeClearance;
    bool const insideCone = distance <= kIngvarSmashConeRadius && boss->HasInArc(kIngvarSmashConeRadians, bot);
    bool const useful = insideBypass || insideCone;
    LOG_DEBUG("playerbots", "Ingvar diagnostic: smash dodge evaluated bot={} distance={:.2f} "
                               "inside_bypass={} inside_cone={} useful={}",
              bot->GetName(), distance, insideBypass, insideCone, useful);
    return useful;
}
bool IngvarDodgeSmashAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss) { return false; }

    // The script roots Ingvar for 3.75 s and disables its rotation for the whole 3 s cast,
    // and the raid-wide stun only lands on resolution, so the rear arc at the melee
    // stand-off satisfies both selection conditions and stays inside the 5.0 yd melee
    // range. The previous implementation walked through the boss to exactly 2.0 yd past
    // its centre, which is the boundary radius itself and left no margin at all.
    // Forced priority: this is a telegraphed lethal hit with a bounded window.
    return MoveBehind(boss, MovementPriority::MOVEMENT_FORCED, "smash_dodge");
}

bool IngvarGetBehindAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss || AI_VALUE(Unit*, "current target") != boss)
        return false;

    return MoveBehind(boss, MovementPriority::MOVEMENT_COMBAT, "ordinary");
}

bool IngvarGetBehindAction::MoveBehind(Unit* boss, MovementPriority priority, char const* reason)
{
    // Effect 0 of Smash / Dark Smash is a 10 yd cone. Melee has to stay in contact and
    // therefore has to answer it by angle, from the rear arc. A member that does not
    // need contact answers it by range instead: leaving the 10 yd radius on its current
    // bearing is the shorter move and is safe at any angle, so ranged bots and healers
    // are no longer walked into the arc behind the boss.
    bool const keepsRange = !botAI->IsTank(bot) && (botAI->IsRanged(bot) || botAI->IsHeal(bot));
    // 近战落点改为 3.5 码：在 2.0 码 boundary 旁路之外、5.0 码近战射程之内。原来的
    // 7.0 码在射程之外，必然被 `reach melee` 再拉回来，这段往返正是近战反复越过
    // 旁路半径的来源。
    float const desiredRange = keepsRange ? kIngvarRangedClearance : kIngvarMeleeStandoff;
    if (keepsRange && bot->GetExactDist2d(boss) >= desiredRange)
        return false;

    float const distance = std::max(0.0f, desiredRange - boss->GetCombatReach());
    float const angle = keepsRange ? boss->GetAngle(bot)
                                   : Position::NormalizeOrientation(boss->GetOrientation() + M_PI);
    bool const bossMovingAtSubmit = boss->isMoving();
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    boss->GetNearPoint(bot, x, y, z, 0.0f, distance, angle);

    if (!bot->GetMap()->CheckCollisionAndGetValidCoords(bot, bot->GetPositionX(), bot->GetPositionY(),
                                                         bot->GetPositionZ(), x, y, z))
    {
        LOG_DEBUG("playerbots", "Ingvar diagnostic: get-behind rejected bot={} reason={} priority={} collision=false",
                  bot->GetName(), reason, static_cast<uint32>(priority));
        return false;
    }

    // The outward variant heads towards the open edges of the Keep platform. Ground and
    // collision already ran; refuse anything that also changes level.
    if (std::fabs(z - bot->GetPositionZ()) > 4.0f)
    {
        LOG_DEBUG("playerbots", "Ingvar diagnostic: get-behind rejected bot={} reason={} priority={} "
                                   "vertical_delta={:.2f}",
                  bot->GetName(), reason, static_cast<uint32>(priority), z - bot->GetPositionZ());
        return false;
    }

    UpdateMovementState();
    bool const canMove = IsMovingAllowed();
    bool const duplicate = canMove && IsDuplicateMove(x, y, z);
    bool const waiting = canMove && !duplicate && IsWaitingForLastMove(priority);
    bool const moved = canMove && !duplicate && !waiting &&
        MoveTo(bot->GetMapId(), x, y, z, false, false, false, true, priority, true);
    LOG_DEBUG("playerbots", "Ingvar diagnostic: get-behind bot={} reason={} priority={} moved={} can_move={} "
                               "duplicate={} "
                               "waiting={} boss_moving={} boss_dest_dist={:.2f} destination=({:.2f},{:.2f},{:.2f})",
              bot->GetName(), reason, static_cast<uint32>(priority), moved, canMove, duplicate, waiting,
              bossMovingAtSubmit, boss->GetExactDist2d(x, y), x, y, z);
    return moved;
}

bool IngvarEvadeDarkSmashAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss)
        return false;

    // This P2 front-cone response is deliberately independent of the bot's
    // attack target: healers and targetless members can still be in danger.
    return MoveBehind(boss, MovementPriority::MOVEMENT_FORCED, "dark_smash_front_cone");
}

bool IngvarEvadeDarkSmashAction::isUseful()
{
    constexpr float kIngvarDarkSmashConeRadians = 1.04719755f;
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    // HasInArc has no range term; effect 0 stops at 10 yd, so a member already outside
    // that radius must not be moved by this response.
    bool const useful = boss && !botAI->IsTank(bot) && boss->GetDisplayId() == INGVAR_UNDEAD_DISPLAY_ID &&
        boss->HasUnitState(UNIT_STATE_ROOT) && boss->HasInArc(kIngvarDarkSmashConeRadians, bot) &&
        bot->GetExactDist2d(boss) <= kIngvarSmashConeRadius;
    LOG_DEBUG("playerbots", "Ingvar diagnostic: dark-smash action useful bot={} useful={}", bot->GetName(), useful);
    return useful;
}

bool IngvarEvadeDarkSmashAction::isPossible()
{
    // Do not use CanMove as a gate here. Execute records its precise rejection
    // reason, including temporary control states, for a triggered response.
    bool const possible = bot->IsAlive() && bot->IsInWorld();
    LOG_DEBUG("playerbots", "Ingvar diagnostic: dark-smash action possible bot={} possible={}",
              bot->GetName(), possible);
    return possible;
}

bool IngvarClearContactAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss || botAI->IsTank(bot) || bot->GetExactDist2d(boss) >= kIngvarMeleeClearance)
        return false;

    return MoveBehind(boss, MovementPriority::MOVEMENT_FORCED, "contact_clearance");
}

bool IngvarClearContactAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    bool const useful = boss && !botAI->IsTank(bot) && bot->GetExactDist2d(boss) < kIngvarMeleeClearance;
    if (useful)
        LOG_DEBUG("playerbots", "Ingvar diagnostic: contact-clearance useful bot={} distance={:.2f}",
                  bot->GetName(), bot->GetExactDist2d(boss));
    return useful;
}

bool IngvarKeepRangeAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss)
        return false;

    return MoveBehind(boss, MovementPriority::MOVEMENT_COMBAT, "ranged_clearance");
}

bool IngvarKeepRangeAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    return boss && !botAI->IsTank(bot) && (botAI->IsRanged(bot) || botAI->IsHeal(bot)) &&
        bot->IsInCombat() && boss->IsInCombat() && bot->GetExactDist2d(boss) < kIngvarRangedClearance;
}

namespace
{
// 点到线段的 2D 距离。散开只校验落点是不够的：bot 走的是直线，途中可以贴到 boss。
float SegmentDistance2d(float ax, float ay, float bx, float by, float px, float py)
{
    float const dx = bx - ax;
    float const dy = by - ay;
    float const lengthSq = dx * dx + dy * dy;
    if (lengthSq <= 0.0f)
        return std::hypot(px - ax, py - ay);

    float t = ((px - ax) * dx + (py - ay) * dy) / lengthSq;
    t = std::clamp(t, 0.0f, 1.0f);
    return std::hypot(px - (ax + t * dx), py - (ay + t * dy));
}
}  // namespace

bool IngvarSpreadAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    Unit* crowd = FindIngvarCrowdingMember(botAI, bot);
    if (!boss || !crowd)
        return false;

    float const distance = bot->GetExactDist2d(crowd);
    // One step that clears the axe radius with a margin, rather than repeated nudges.
    float const step = std::max(3.0f, kIngvarSpreadRadius + 2.0f - distance);
    // 两名成员完全重叠时 `crowd->GetAngle(bot)` 是 `atan2(0, 0)`，恒为 0——散开方向
    // 退化成「正东」，与 boss 在哪无关。run317/seq4 的治疗正是这样：与法师重叠
    // （日志 `distance=0.00 step=10.00`）后朝正东走，而 boss 就在正东。重叠时改用
    // 「背离 boss」作为基准方向，这个方向任何时候都是安全的。
    constexpr float kDegenerateOverlap = 0.5f;
    float const angle = distance > kDegenerateOverlap ? crowd->GetAngle(bot) : boss->GetAngle(bot);
    // Away from the crowding member first. On a platform with open edges and four other
    // members that candidate can be invalid, so fan out through the same validation.
    static constexpr float kOffsets[] = { 0.0f, float(M_PI_4), -float(M_PI_4), float(M_PI_2), -float(M_PI_2) };

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool selected = false;
    float selectedBossDistance = 0.0f;
    Unit* const losAnchor = FindIngvarLosAnchor(botAI, bot);
    uint32 losRejected = 0;
    for (float offset : kOffsets)
    {
        float const candidateAngle = angle + offset;
        float candidateX = bot->GetPositionX() + cos(candidateAngle) * step;
        float candidateY = bot->GetPositionY() + sin(candidateAngle) * step;
        float candidateZ = bot->GetMapWaterOrGroundLevel(candidateX, candidateY, bot->GetPositionZ());
        bool const validGround = candidateZ != INVALID_HEIGHT && candidateZ != -100000.0f &&
            candidateZ != -200000.0f && std::fabs(candidateZ - bot->GetPositionZ()) <= 4.0f;
        if (!validGround || !bot->GetMap()->CheckCollisionAndGetValidCoords(bot, bot->GetPositionX(),
                                                                            bot->GetPositionY(), bot->GetPositionZ(),
                                                                            candidateX, candidateY, candidateZ))
            continue;

        float const crowdDistance = crowd->GetExactDist2d(candidateX, candidateY);
        float const bossDistance = boss->GetExactDist2d(candidateX, candidateY);
        // Separation must actually improve, the candidate must stay outside the smash
        // cone, and it must not push the bot past its own working range: spreading is
        // not allowed to buy safety by dropping out of the fight. A bot already beyond
        // that range keeps its current distance as the ceiling instead of being pulled in.
        float const rangeCeiling = std::max(bot->GetExactDist2d(boss),
            (botAI->IsHeal(bot) ? sPlayerbotAIConfig.healDistance : sPlayerbotAIConfig.spellDistance) - 2.0f);
        if (crowdDistance <= distance || bossDistance < kIngvarRangedClearance || bossDistance > rangeCeiling)
            continue;

        // 落点合格不代表路上合格：提交的是 direct route，bot 走直线。run317/seq4 的
        // 治疗落点 boss 距离达标，却在途中贴到 boss 1.38 码——穿过了 2.0 码 boundary
        // 旁路（`IsWithinBoundaryRadius` 用纯 3D 中心距，角度判定被完全绕过），随后
        // 被磨到 1,670/16,500 并被 Dreadful Roar 收掉，是那一场唯一的团灭起点。
        // 散开是站位调整，不该让人在途中比出发时更靠近 boss。
        float const pathClearance = std::min(bot->GetExactDist2d(boss), kIngvarRangedClearance);
        if (SegmentDistance2d(bot->GetPositionX(), bot->GetPositionY(), candidateX, candidateY,
                              boss->GetPositionX(), boss->GetPositionY()) < pathClearance)
            continue;

        // 散开不得把自己挪进柱子影里。落点几何合法不等于还能"看见"主坦：
        // `PartyMemberValue::Check` / `PartyMemberToHeal::Check` 用 `IsWithinLOS` 过滤候选，
        // 看不见主坦时治疗的 `party member to heal`、法师的 `party member to dispel`
        // 都静默返回空——不治、不解，连"走过去"的动作也拿不到目标（run528/seq4）。
        if (!IngvarPointHasLosTo(bot, losAnchor, candidateX, candidateY, candidateZ))
        {
            ++losRejected;
            continue;
        }

        x = candidateX;
        y = candidateY;
        z = candidateZ;
        selected = true;
        selectedBossDistance = bossDistance;
        break;
    }

    // Standing formation, not an emergency: combat priority so the axe evade and the
    // front-cone response can still preempt it.
    bool const moved = selected &&
        MoveTo(bot->GetMapId(), x, y, z, false, false, true, true, MovementPriority::MOVEMENT_COMBAT, true);
    LOG_DEBUG("playerbots", "Ingvar diagnostic: spread bot={} crowd={} distance={:.2f} step={:.2f} selected={} "
                               "moved={} destination=({:.2f},{:.2f},{:.2f}) boss_dest_dist={:.2f} los_rejected={}",
              bot->GetName(), crowd->GetName(), distance, step, selected, moved, x, y, z, selectedBossDistance,
              losRejected);
    return moved;
}

bool IngvarSpreadAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    return boss && !botAI->IsTank(bot) && bot->IsInCombat() && boss->IsInCombat() &&
        (botAI->IsRanged(bot) || botAI->IsHeal(bot)) && FindIngvarCrowdingMember(botAI, bot) != nullptr;
}

bool IngvarRegainLosAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    if (!boss || botAI->IsTank(bot) || (!botAI->IsRanged(bot) && !botAI->IsHeal(bot)))
        return false;
    if (!bot->IsInCombat() || !boss->IsInCombat())
        return false;

    Unit* anchor = FindIngvarLosAnchor(botAI, bot);
    return anchor && anchor != bot && !IngvarHasLosTo(bot, anchor);
}

bool IngvarRegainLosAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    Unit* anchor = FindIngvarLosAnchor(botAI, bot);
    if (!boss || !anchor)
        return false;

    // 侧移绕开遮挡，而不是朝主坦直线跑：主坦就站在 boss 身上，直线接近等于把治疗
    // 送进猛击锥。先试小半径的八个方向，再放大——取第一个"位移最小且能看见锚点"的点。
    static constexpr float kRadii[] = { 6.0f, 10.0f, 15.0f };
    static constexpr int kDirections = 8;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool selected = false;
    uint32 groundRejected = 0;
    uint32 bossRejected = 0;
    uint32 losRejected = 0;

    for (float radius : kRadii)
    {
        for (int direction = 0; direction != kDirections && !selected; ++direction)
        {
            float const angle = bot->GetOrientation() + direction * (2.0f * float(M_PI) / kDirections);
            float candidateX = bot->GetPositionX() + cos(angle) * radius;
            float candidateY = bot->GetPositionY() + sin(angle) * radius;
            float candidateZ = bot->GetMapWaterOrGroundLevel(candidateX, candidateY, bot->GetPositionZ());
            bool const validGround = candidateZ != INVALID_HEIGHT && candidateZ != -100000.0f &&
                candidateZ != -200000.0f && std::fabs(candidateZ - bot->GetPositionZ()) <= 4.0f;
            if (!validGround || !bot->GetMap()->CheckCollisionAndGetValidCoords(bot, bot->GetPositionX(),
                                                                                bot->GetPositionY(),
                                                                                bot->GetPositionZ(),
                                                                                candidateX, candidateY, candidateZ))
            {
                ++groundRejected;
                continue;
            }

            // 恢复视线不能以走进猛击锥为代价，路径同样不许比出发时更贴近 boss。
            float const bossDistance = boss->GetExactDist2d(candidateX, candidateY);
            float const pathClearance = std::min(bot->GetExactDist2d(boss), kIngvarRangedClearance);
            if (bossDistance < kIngvarRangedClearance ||
                SegmentDistance2d(bot->GetPositionX(), bot->GetPositionY(), candidateX, candidateY,
                                  boss->GetPositionX(), boss->GetPositionY()) < pathClearance)
            {
                ++bossRejected;
                continue;
            }

            if (!IngvarPointHasLosTo(bot, anchor, candidateX, candidateY, candidateZ))
            {
                ++losRejected;
                continue;
            }

            x = candidateX;
            y = candidateY;
            z = candidateZ;
            selected = true;
        }
        if (selected)
            break;
    }

    bool const moved = selected &&
        MoveTo(bot->GetMapId(), x, y, z, false, false, true, true, MovementPriority::MOVEMENT_COMBAT, true);
    LOG_DEBUG("playerbots", "Ingvar diagnostic: regain los bot={} anchor={} anchor_dist={:.2f} selected={} moved={} "
                               "destination=({:.2f},{:.2f},{:.2f}) rejected_ground={} rejected_boss={} rejected_los={}",
              bot->GetName(), anchor->GetName(), bot->GetExactDist2d(anchor), selected, moved, x, y, z,
              groundRejected, bossRejected, losRejected);
    return moved;
}

bool IngvarAvoidShadowAxeAction::Execute(Event /*event*/)
{
    constexpr float kIngvarBossClearance = 7.0f;
    Unit* boss = AI_VALUE2(Unit*, "find target", "ingvar the plunderer");
    std::list<Creature*> axes;
    bot->GetCreatureListWithEntryInGrid(axes, NPC_THROW, 20.0f);

    Creature* nearest = nullptr;
    for (Creature* axe : axes)
        if (axe && axe->IsAlive() && (!nearest || bot->GetExactDist2d(axe) < bot->GetExactDist2d(nearest)))
            nearest = axe;

    if (!nearest)
        return false;

    float const distance = bot->GetExactDist2d(nearest);
    // A thrown axe can cross the same bot's path more than once while travelling
    // out and back. Do not remember a GUID as "handled": that suppresses every
    // later evade from that still-dangerous axe. Unlike the generic MoveAway,
    // resolve a ground height before committing the combat movement: the Keep
    // platform has open edges where a horizontal-only candidate makes a bot fall.
    float const angle = nearest->GetAngle(bot);
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool primaryGround = false;
    bool primaryCollision = false;
    float primaryDisplacement = 0.0f;
    int selectedDirection = -1;

    // Directly fleeing from the axe is preferred. On the uneven platform Detour
    // can validly raycast that vector back to the bot's own point, which is not a
    // movement at all. In that narrow case try either perpendicular point using
    // the exact same ground and collision validation; both still increase the
    // distance from a nearby axe and never bypass normal movement.
    for (int direction = 0; direction != 3 && selectedDirection == -1; ++direction)
    {
        float const candidateAngle = angle + (direction == 1 ? M_PI_2 : direction == 2 ? -M_PI_2 : 0.0f);
        float candidateX = bot->GetPositionX() + cos(candidateAngle) * 8.0f;
        float candidateY = bot->GetPositionY() + sin(candidateAngle) * 8.0f;
        float candidateZ = bot->GetMapWaterOrGroundLevel(candidateX, candidateY, bot->GetPositionZ());
        bool const validGround = candidateZ != INVALID_HEIGHT && candidateZ != -100000.0f && candidateZ != -200000.0f;
        bool const validCollision = validGround &&
            bot->GetMap()->CheckCollisionAndGetValidCoords(bot, bot->GetPositionX(), bot->GetPositionY(),
                                                            bot->GetPositionZ(), candidateX, candidateY, candidateZ);
        float const displacement = validCollision ? bot->GetExactDist(candidateX, candidateY, candidateZ) : 0.0f;

        if (direction == 0)
        {
            primaryGround = validGround;
            primaryCollision = validCollision;
            primaryDisplacement = displacement;
        }

        float const axeDistanceAtCandidate = nearest->GetExactDist2d(candidateX, candidateY);
        float const bossDistanceAtCandidate = boss ? boss->GetExactDist2d(candidateX, candidateY) : kIngvarBossClearance;
        if (validCollision && displacement > 0.25f && axeDistanceAtCandidate > distance &&
            bossDistanceAtCandidate >= kIngvarBossClearance)
        {
            x = candidateX;
            y = candidateY;
            z = candidateZ;
            selectedDirection = direction;
        }
    }

    // Keep this diagnostic local to the lethal range. MoveTo intentionally has several
    // early exits; recording which one wins avoids changing a second behavior blindly.
    bool canMove = false;
    bool duplicate = false;
    bool waiting = false;
    if (selectedDirection != -1)
    {
        UpdateMovementState();
        canMove = IsMovingAllowed();
        duplicate = canMove && IsDuplicateMove(x, y, z);
        waiting = canMove && !duplicate && IsWaitingForLastMove(MovementPriority::MOVEMENT_FORCED);
    }

    bool const moved = selectedDirection != -1 && canMove && !duplicate && !waiting &&
        // The axe is a short-lived lethal hazard. Combat-priority movement can be held behind
        // an earlier chase/positioning command while the axe reaches its random target; forced
        // priority is limited to this validated evade and may preempt that stale movement.
        MoveTo(bot->GetMapId(), x, y, z, false, false, true, true, MovementPriority::MOVEMENT_FORCED, true);
    float const bossDistanceAtDestination = boss && selectedDirection != -1 ? boss->GetExactDist2d(x, y) : -1.0f;
    LOG_DEBUG("playerbots", "Ingvar diagnostic: shadow axe evade bot={} axe={} distance={:.2f} moved={} "
                               "bot=({:.2f},{:.2f},{:.2f}) axe=({:.2f},{:.2f},{:.2f}) "
                               "destination=({:.2f},{:.2f},{:.2f}) boss_dest_dist={:.2f}",
             bot->GetName(), nearest->GetGUID().ToString(), distance, moved,
             bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
             nearest->GetPositionX(), nearest->GetPositionY(), nearest->GetPositionZ(), x, y, z, bossDistanceAtDestination);
    if (!moved && distance <= 6.0f)
        LOG_DEBUG("playerbots", "Ingvar diagnostic: shadow axe move rejected bot={} axe={} primary_ground={} "
                               "primary_collision={} primary_displacement={:.2f} direction={} can_move={} "
                               "duplicate={} waiting={} destination=({:.2f},{:.2f},{:.2f}) movement_state={}",
                 bot->GetName(), nearest->GetGUID().ToString(), primaryGround, primaryCollision, primaryDisplacement,
                 selectedDirection, canMove, duplicate, waiting, x, y, z, DescribeIngvarMovementState(botAI, bot));
    return moved;
}

bool IngvarAvoidShadowAxeAction::isUseful()
{
    std::list<Creature*> axes;
    bot->GetCreatureListWithEntryInGrid(axes, NPC_THROW, 20.0f);

    Creature* nearest = nullptr;
    for (Creature* axe : axes)
        if (axe && axe->IsAlive() && (!nearest || bot->GetExactDist2d(axe) < bot->GetExactDist2d(nearest)))
            nearest = axe;

    if (!nearest)
    {
        _lastLoggedAxe.Clear();
        return false;
    }

    float const distance = bot->GetExactDist2d(nearest);
    bool const useful = distance <= kIngvarShadowAxeActionRadius;
    if (_lastLoggedAxe != nearest->GetGUID())
    {
        _lastLoggedAxe = nearest->GetGUID();
        LOG_DEBUG("playerbots", "Ingvar diagnostic: shadow axe seen bot={} axe={} distance={:.2f} "
                               "within_action_radius={}",
                 bot->GetName(), nearest->GetGUID().ToString(), distance, useful);
    }

    return useful;
}

