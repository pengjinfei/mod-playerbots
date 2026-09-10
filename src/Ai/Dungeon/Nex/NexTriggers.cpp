/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NexTriggers.h"
#include "AiObject.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

bool FactionCommanderWhirlwindTrigger::IsActive()
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

    if (boss && boss->HasUnitState(UNIT_STATE_CASTING))
    {
        if (boss->FindCurrentSpellBySpellId(SPELL_WHIRLWIND))
        {
            return true;
        }
    }
    return false;
}

bool TelestraFirebombTrigger::IsActive()
{
    if (botAI->IsMelee(bot)) { return false; }

    Unit* boss = AI_VALUE2(Unit*, "find target", "grand magus telestra");
    // Avoid split phase with the fake Telestra units, only match the true boss id
    return boss && boss->GetEntry() == NPC_TELESTRA;
}

bool TelestraSplitPhaseTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand magus telestra");
    // Only match split phase with the fake Telestra units
    return boss && boss->GetEntry() != NPC_TELESTRA;
}

Unit* FindNearestChaoticRift(PlayerbotAI* botAI, Player* bot, AiObjectContext* context)
{
    // "possible targets no los" 的半径是 sightDistance（本机配置 100 码），而阿诺姆鲁斯房间
    // 之外还有 6 个 DB 预置的 Chaotic Rift，最近的距 boss 81.6 码——一旦被 Charge Rifts
    // 点亮就会出现在候选里。加这个上限，避免场上没有召唤裂隙时 bot 跑去打房外的那些。
    // 45 码略大于最远法术射程（40 码），召唤裂隙生成在 boss 边上，一定落在这个范围内。
    constexpr float kMaxRiftDistance = 45.0f;

    Unit* closest = nullptr;
    float closestDistance = 0.0f;

    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        // 按 entry 判定而不是按名字：普通 26918 与英雄 30522 同名不同档，
        // 且名字匹配对本地化/后缀敏感。
        if (unit->GetEntry() != NPC_CHAOTIC_RIFT && unit->GetEntry() != NPC_CHAOTIC_RIFT_HEROIC)
            continue;

        float const distance = bot->GetDistance(unit);
        if (distance > kMaxRiftDistance)
            continue;

        if (!closest || distance < closestDistance)
        {
            closest = unit;
            closestDistance = distance;
        }
    }

    return closest;
}

bool ChaoticRiftTrigger::IsActive()
{
    // 判据从「boss 挂着裂隙护盾」改为「场上有存活裂隙」。
    // 原判据只在护盾期转火，而英雄阿诺姆鲁斯每 15 秒生成一个裂隙、每个裂隙每 5 秒和
    // 每 10 秒各召唤一只奥术怨魂（SmartAI 26918 的 id 2 与 id 6），护盾又只在
    // activeRifts 归零时才解除（boss_anomalus.cpp 的 SetData）。实测 run342 五场：
    // 裂隙每 15.9/30.9/45.9/60.9 秒各生成一个，前两个约 12 秒被打掉，第三个之后再没死过
    // （单个裂隙约 30k 血，第三个 30 秒只吃到 23,229 伤害），每场累计 16–20 只怨魂；
    // boss 掉到 51% 以下挂盾后血量就此不动（attempt1 从 55 秒起 31 秒零伤害），
    // 直到全队在 76–86 秒崩盘，boss 停在 33–45%。
    // 所以裂隙必须一出现就转火，而不是等护盾。
    return FindNearestChaoticRift(botAI, bot, context) != nullptr;
}

bool OrmorokSpikesTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ormorok the tree-shaper");
    if (!boss || !botAI->IsTank(bot)) { return false; }

    GuidVector objects = AI_VALUE(GuidVector, "closest game objects");
    for (auto i = objects.begin(); i != objects.end(); ++i)
    {
        GameObject* go = botAI->GetGameObject(*i);
        if (go && go->GetEntry() == GO_CRYSTAL_SPIKE)
        {
            return true;
        }
    }
    return false;
}

bool OrmorokStackTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "ormorok the tree-shaper");
    return (boss && !botAI->IsTank(bot));
}

bool IntenseColdTrigger::IsActive()
{
    // Adjust as needed - too much interrupting loses dps time,
    // but too many stacks is deadly. Assuming 3-5 is a good number to clear
    int stackThreshold = 5;
    Unit* boss = AI_VALUE2(Unit*, "find target", "keristrasza");
    return boss && botAI->GetAura("intense cold", bot, false, false, stackThreshold);
}

bool KeristraszaPositioningTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "keristrasza");
    // Include healers here for now, otherwise they stand in things
    return boss && !botAI->IsTank(bot) && !botAI->IsRangedDps(bot);
    // return boss && botAI->IsMelee(bot) && !botAI->IsTank(bot);
}
