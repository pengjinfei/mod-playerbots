/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NexActions.h"
#include "NexTriggers.h"
#include "Playerbots.h"

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
Unit* TrashHealerCcAction::GetTarget()
{
    Unit* target = FindCcableTrashHealer(botAI, bot, context, spell, casterClass, farthest);
    if (!target)
        return nullptr;

    auto const itr = lastCast.find(target->GetGUID());
    if (itr != lastCast.end() && getMSTimeDiff(itr->second, getMSTime()) < kRetryCooldownMs)
        return nullptr;

    return target;
}

bool TrashHealerCcAction::Execute(Event event)
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    ObjectGuid const guid = target->GetGUID();
    if (!CastSpellAction::Execute(event))
        return false;

    lastCast[guid] = getMSTime();
    return true;
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
