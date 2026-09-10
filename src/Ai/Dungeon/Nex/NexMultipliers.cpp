/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NexMultipliers.h"
#include "ChooseTargetActions.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "NexActions.h"
#include "NexTriggers.h"

float FactionCommanderMultiplier::GetValue(Action* action)
{
    Unit* boss = nullptr;
    uint8 faction = bot->GetTeamId();

    switch (bot->GetMap()->GetDifficulty())
    {
        case DUNGEON_DIFFICULTY_NORMAL:
            if (faction == TEAM_ALLIANCE)
            {
                boss = AI_VALUE2(Unit*, "find target", "horde commander");
            }
            else //if (faction == TEAM_HORDE)
            {
                boss = AI_VALUE2(Unit*, "find target", "alliance commander");
            }
            break;
        case DUNGEON_DIFFICULTY_HEROIC:
            if (faction == TEAM_ALLIANCE)
            {
                boss = AI_VALUE2(Unit*, "find target", "commander kolurg");
            }
            else //if (faction == TEAM_HORDE)
            {
                boss = AI_VALUE2(Unit*, "find target", "commander stoutbeard");
            }
            break;
        default:
            break;
    }
    if (boss && boss->HasUnitState(UNIT_STATE_CASTING) &&
        boss->FindCurrentSpellBySpellId(SPELL_WHIRLWIND))
    {
        // Prevent movement actions other than flee during a whirlwind, to prevent running back in early.
        if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<MoveFromWhirlwindAction*>(action))
        {
            return 0.0f;
        }
    }
    return 1.0f;
}

float TelestraMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand magus telestra");
    if (boss && boss->GetEntry() != NPC_TELESTRA)
    {
        // boss is split into clones, do not auto acquire target
        if (dynamic_cast<DpsAssistAction*>(action))
        {
            return 0.0f;
        }
    }
    return 1.0f;
}

float AnomalusMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anomalus");
    if (!boss)
        return 1.0f;

    if (!dynamic_cast<DpsAssistAction*>(action))
        return 1.0f;

    // 护盾期 boss 免疫，自动选怪必须让路（原有行为）。
    if (boss->HasAura(BUFF_RIFT_SHIELD))
        return 0.0f;

    // 裂隙存活期也要让路，但只对非坦克：ChaoticRiftTrigger 的判据放宽到「场上有存活裂隙」
    // 之后，转火会发生在 boss 可被攻击的窗口里，此时 dps assist 每个 tick 都会把目标
    // 拉回坦克的目标（boss），与 ACTION_RAID 的转火动作来回顶，裂隙一直打不死。
    // 坦克不受影响，必须继续抓 boss 仇恨。
    if (!botAI->IsTank(bot) && FindNearestChaoticRift(botAI, bot, context))
        return 0.0f;

    return 1.0f;
}

float OrmorokMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ormorok the tree-shaper");
    if (!boss) { return 1.0f; }

    // These are used for auto ranged repositioning, need to suppress so ranged dps don't ping-pong
    if (dynamic_cast<FleeAction*>(action))
    {
        return 0.0f;
    }
    // This boss is annoying and shuffles around a lot. Don't let tank move once fight has started.
    // Extra checks are to allow the tank to close distance and engage the boss initially
    if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<DodgeSpikesAction*>(action)
        && botAI->IsTank(bot) && bot->IsWithinMeleeRange(boss)
        && AI_VALUE2(bool, "facing", "current target"))
        {
            return 0.0f;
        }
    return 1.0f;
}
