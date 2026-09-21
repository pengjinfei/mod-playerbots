/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GDTriggers.h"
#include "AiObjectContext.h"
#include "AttackersValue.h"
#include "Creature.h"
#include "EncounterHelpers.h"
#include "Playerbots.h"
#include <algorithm>
#include <list>
#include <utility>
#include <vector>

using namespace EncounterHelpers;

namespace GundrakSladran
{

static bool IsClosestSnakeWrapFreer(Player* bot, Unit* snakeWrap, float distance)
{
    Group* group = bot->GetGroup();
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive() || !member->IsInCombat() ||
            !GET_PLAYERBOT_AI(member) || member->GetMap() != bot->GetMap() ||
            !PlayerbotAI::IsDps(member) || member->HasAura(SPELL_SNAKE_WRAP))
        {
            continue;
        }

        float memberDistance = member->GetExactDist2d(snakeWrap);
        if (memberDistance > distance ||
            (memberDistance == distance && bot->GetGUID() < member->GetGUID()))
        {
            continue;
        }

        if (!member->IsWithinLOSInMap(snakeWrap))
        {
            continue;
        }

        return false;
    }

    return true;
}

bool IsAdd(Unit* unit)
{
    if (!unit) { return false; }

    uint32 entry = unit->GetEntry();
    return entry == NPC_SLADRAN_VIPER || entry == NPC_SLADRAN_CONSTRICTOR;
}

ObjectGuid CalculateAssignedSnakeWrap(PlayerbotAI* botAI)
{
    Player* bot = botAI->GetBot();
    if (!bot->IsAlive() || !bot->IsInCombat() || !PlayerbotAI::IsDps(bot) ||
        bot->HasAura(SPELL_SNAKE_WRAP) || !bot->GetGroup())
    {
        return ObjectGuid::Empty;
    }

    std::list<Creature*> wraps;
    bot->GetCreatureListWithEntryInGrid(wraps, NPC_SNAKE_WRAP, sPlayerbotAIConfig.sightDistance);

    std::vector<std::pair<float, Creature*>> candidates;
    candidates.reserve(wraps.size());
    for (Creature* snakeWrap : wraps)
    {
        if (!snakeWrap->IsAlive() || !AttackersValue::IsPossibleTarget(snakeWrap, bot)) { continue; }

        candidates.emplace_back(bot->GetExactDist2d(snakeWrap), snakeWrap);
    }

    std::sort(candidates.begin(), candidates.end(),
              [](std::pair<float, Creature*> const& lhs, std::pair<float, Creature*> const& rhs)
              { return lhs.first < rhs.first; });

    for (auto const& candidate : candidates)
    {
        if (!bot->IsWithinLOSInMap(candidate.second)) { continue; }

        if (IsClosestSnakeWrapFreer(bot, candidate.second, candidate.first))
        {
            return candidate.second->GetGUID();
        }
    }

    return ObjectGuid::Empty;
}

Unit* GetAssignedSnakeWrap(PlayerbotAI* botAI)
{
    ObjectGuid guid =
        botAI->GetAiObjectContext()->GetValue<ObjectGuid>("slad'ran snake wrap target")->Get();
    if (guid.IsEmpty()) { return nullptr; }

    Unit* snakeWrap = botAI->GetUnit(guid);
    if (!snakeWrap || !snakeWrap->IsAlive() || snakeWrap->GetEntry() != NPC_SNAKE_WRAP)
    {
        return nullptr;
    }

    return snakeWrap;
}

Player* GetStackTank(PlayerbotAI* botAI)
{
    Player* bot = botAI->GetBot();
    if (!bot->IsAlive() || PlayerbotAI::IsTank(bot))
    {
        return nullptr;
    }

    AiObjectContext* context = botAI->GetAiObjectContext();
    if (!context->GetValue<Unit*>("find target", "slad'ran")->Get())
    {
        return nullptr;
    }

    if (GetAssignedSnakeWrap(botAI))
    {
        return nullptr;
    }

    Unit* currentTarget = context->GetValue<Unit*>("current target")->Get();
    if (currentTarget && currentTarget->IsAlive() && currentTarget->GetEntry() == NPC_SNAKE_WRAP)
    {
        return nullptr;
    }

    Player* tank = GetGroupMainTank(bot);
    if (!tank || tank == bot || tank->GetMap() != bot->GetMap())
    {
        return nullptr;
    }

    if (bot->GetExactDist2d(tank) <= STACK_LEASH_YD)
    {
        return nullptr;
    }

    return tank;
}

Unit* GetTankHoldTarget(PlayerbotAI* botAI)
{
    Player* bot = botAI->GetBot();
    if (!bot->IsAlive() || !PlayerbotAI::IsTank(bot))
    {
        return nullptr;
    }

    AiObjectContext* context = botAI->GetAiObjectContext();
    Unit* boss = context->GetValue<Unit*>("find target", "slad'ran")->Get();
    if (!boss || !boss->IsAlive())
    {
        return nullptr;
    }

    Unit* currentTarget = context->GetValue<Unit*>("current target")->Get();
    if (currentTarget && currentTarget->IsAlive())
    {
        if (currentTarget == boss)
        {
            return nullptr;
        }

        if (IsAdd(currentTarget) && bot->GetExactDist2d(currentTarget) <= TANK_PICKUP_YD)
        {
            return nullptr;
        }
    }

    return boss;
}

}

bool SladranPoisonNovaTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "slad'ran");
    if (!boss) { return false; }

    return bool(boss->FindCurrentSpellBySpellId(SPELL_POISON_NOVA));
}

bool SladranSnakeWrapTrigger::IsActive()
{
    return GundrakSladran::GetAssignedSnakeWrap(botAI) != nullptr;
}

bool SladranStackOnTankTrigger::IsActive()
{
    return GundrakSladran::GetStackTank(botAI) != nullptr;
}

bool SladranTankHoldTrigger::IsActive()
{
    return GundrakSladran::GetTankHoldTarget(botAI) != nullptr;
}

bool SladranFocusBossTrigger::IsActive()
{
    if (!botAI->IsDps(bot)) { return false; }

    Unit* boss = AI_VALUE2(Unit*, "find target", "slad'ran");
    if (!boss || !boss->IsAlive()) { return false; }

    Unit* current = AI_VALUE(Unit*, "current target");
    if (current == boss) { return false; }

    // 正在打包裹就别打断——第二刀的 A/B 已经证明「半路丢下包裹」是净负面
    // （包裹被打死 3.9 → 0.8 只/场，场次时长 −19%，击杀 2/10 → 0/5）。
    if (current && current->GetEntry() == NPC_SNAKE_WRAP && current->IsAlive()) { return false; }

    return true;
}

bool GaldarahWhirlingSlashTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gal'darah");
    return boss && boss->HasAura(SPELL_WHIRLING_SLASH);
}

bool MoorabiLancerRetaliationTrigger::IsActive()
{
    if (bot->getClass() != CLASS_ROGUE)
        return false;

    Unit* target = bot->GetVictim();
    if (!target)
        target = AI_VALUE(Unit*, "current target");
    return target && target->GetEntry() == NPC_DRAKKARI_LANCER && target->HasAura(SPELL_LANCER_RETALIATION);
}
