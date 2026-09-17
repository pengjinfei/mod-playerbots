/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GDTriggers.h"
#include "AiObjectContext.h"
#include "Playerbots.h"

bool SladranPoisonNovaTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "slad'ran");
    if (!boss) { return false; }

    return bool(boss->FindCurrentSpellBySpellId(SPELL_POISON_NOVA));
}

bool SladranSnakeWrapTrigger::IsActive()
{
    if (!botAI->IsDps(bot)) { return false; }

    // Target is not findable from threat table using AI_VALUE2(),
    // therefore need to search manually for the unit name
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");

    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->GetEntry() == NPC_SNAKE_WRAP)
        {
            return true;
        }
    }
    return false;
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
