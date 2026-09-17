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

bool SladranFocusViperTrigger::IsActive()
{
    if (!botAI->IsDps(bot)) { return false; }

    Unit* boss = AI_VALUE2(Unit*, "find target", "slad'ran");
    if (!boss || !boss->IsAlive()) { return false; }

    Unit* current = AI_VALUE(Unit*, "current target");
    // 已经在打红蛇 / 在打包裹（第二刀证明半路丢下包裹是净负面）就别换
    if (current && current->IsAlive() &&
        (current->GetEntry() == NPC_SLADRAN_VIPER || current->GetEntry() == NPC_SNAKE_WRAP))
        return false;

    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");
    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->IsAlive() && unit->GetEntry() == NPC_SLADRAN_VIPER)
            return true;
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
    //
    // 2026-09-17 第十刀修：活红蛇也必须排除。否则「活红蛇」对本触发器算
    // 「既不是 boss 也不是包裹」→ 本节点亮起把 bot 从红蛇上拉走，
    // 下一 tick `slad'ran focus viper`(56) 相关性更高又拉回来 —— 每 tick 拉锯，
    // 而换目标要 4 个 tick≈2.8 秒，实测总输出速率直接崩 65%
    // （5,176/秒 → 1,792/秒，run 622 两场）。
    if (current && current->IsAlive() &&
        (current->GetEntry() == NPC_SNAKE_WRAP || current->GetEntry() == NPC_SLADRAN_VIPER))
        return false;

    return true;
}

bool GaldarahWhirlingSlashTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gal'darah");
    return boss && boss->HasAura(SPELL_WHIRLING_SLASH);
}
