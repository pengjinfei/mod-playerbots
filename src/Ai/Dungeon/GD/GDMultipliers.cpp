/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GDMultipliers.h"
#include "Action.h"
#include "ChooseTargetActions.h"
#include "GDActions.h"
#include "GDTriggers.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"

float SladranMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "slad'ran");
    if (!boss) { return 1.0f; }

    if (boss->FindCurrentSpellBySpellId(SPELL_POISON_NOVA))
    {
        if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<AvoidPoisonNovaAction*>(action))
        {
            return 0.0f;
        }
    }

    if (!botAI->IsDps(bot)) { return 1.0f; }

    // 2026-09-16：删掉了原来的「DPS 全程禁 AoE」这一段（`getThreatType() == Aoe -> 0.0f`）。
    //
    // 上游在 GDStrategy.cpp 自己写着 "Will re-test in heroic" —— 英雄难度从没测过。
    // 英雄实测（mod-raidtest run 577/581，heroic-gd-sladran-disc-n5）：
    //   - AC 的 boss_slad_ran.cpp 里，血量 ≤90% 起每 8 秒召 2 只毒蛇、≤75%（英雄）起
    //     每 3–5 秒召 3 只缠绕者，**没有上限**；每场有 33–69 只小怪活到团灭。
    //   - 这一段把三个 DPS 的 Aoe 型动作两场归零 462 次、实际只放出 5 次。
    //     暴风雪 101/124、烈焰风暴 101/124、刀扇 66/278 的 `IMPOSSIBLE` 计数与
    //     「被本乘子归零」的计数**完全相等** —— 一次施法失败都没有，
    //     也就是说本乘子是法师整场不放暴风雪/烈焰风暴的唯一原因。
    //   - 小怪是非精英、6517 血；刷怪速率 0.75 只/秒 = 4888 HP/秒，
    //     而全队单体总输出只有 4.5–5.5k/秒 —— 单体点杀清不完（实测只打死 16–42%）。
    //     AoE 的产出随命中数放大，场上常驻 30–50 只，只要 5 只以上进圈就能追平刷怪速率。
    //     **AoE 是唯一追得上刷怪的手段，而它恰好就是这一段关掉的那个。**
    //
    // 保留下面「场上有 Snake Wrap 时禁止 DpsAssistAction」那一段不动：它确实让
    // 44–46% 的战斗时间里 DPS 的目标 100% 不在 boss 身上，但打包裹本身是正确打法，
    // 而且 `attack snake wrap`(相关性 64) 本来就压过 `dps assist`(50)、这条乘子是冗余的 ——
    // 收益不如本次改动确定，按「一次只改一个变量」留到下一轮。

    Unit* snakeWrap = nullptr;
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");
    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->GetEntry() == NPC_SNAKE_WRAP)
        {
            snakeWrap = unit;
            break;
        }
    }
    // Prevent auto-target acquisition during snake wraps
    if (snakeWrap && dynamic_cast<DpsAssistAction*>(action))
    {
        return 0.0f;
    }

    return 1.0f;
}

float GaldarahMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gal'darah");
    if (!boss) { return 1.0f; }

    if (boss->HasAura(SPELL_WHIRLING_SLASH))
        {
            if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<AvoidWhirlingSlashAction*>(action))
            {
                return 0.0f;
            }
        }
    return 1.0f;
}
