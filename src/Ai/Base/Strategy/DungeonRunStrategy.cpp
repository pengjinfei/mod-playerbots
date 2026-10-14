/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "DungeonRunStrategy.h"

#include "Group.h"
#include "ObjectAccessor.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"

void DungeonRunStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("dungeon run", { NextAction("dungeon run advance", ACTION_MOVE) }));
    triggers.push_back(
        new TriggerNode("dungeon run group held", { NextAction("dungeon run release group", ACTION_EMERGENCY) }));
}

bool DungeonRunStrategy::CanAssistTarget(PlayerbotAI* botAI, Unit* target)
{
    if (!botAI || !target || target->IsInCombat())
        return true;

    Player* bot = botAI->GetBot();
    if (!bot || !bot->IsInWorld() || !bot->GetGroup() || PlayerbotAI::IsTank(bot))
        return true;
    // Only the opener is the tank's: once the group fights, targets that never enter combat themselves (Jedoga's
    // volunteers walking to the altar) still have to be killed by the rest.
    if (TrashCcGroupInCombat(bot))
        return true;

    Group* group = bot->GetGroup();
    Player* leader = ObjectAccessor::GetPlayer(*bot, group->GetLeaderGUID());
    if (!leader || leader->GetGroup() != group)
        return true;

    PlayerbotAI* leaderAI = GET_PLAYERBOT_AI(leader);
    // Assist must not start the next pack before the tank pulls (Ahn'kahet pack 262, run 2461).
    return !leaderAI || (!leaderAI->HasStrategy("dungeon run", BOT_STATE_NON_COMBAT) &&
                         !leaderAI->HasStrategy("dungeon run", BOT_STATE_COMBAT));
}
