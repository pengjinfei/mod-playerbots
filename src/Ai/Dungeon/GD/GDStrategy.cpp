/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GDStrategy.h"
#include "GDMultipliers.h"

void WotlkDungeonGDStrategy::InitTriggers(std::vector<TriggerNode*> &triggers)
{
    // Moorabi

    // Drakkari Colossus

    // Slad'ran
    // TODO: Might need to add target priority for heroic on the snakes or to burn down boss.
    // Will re-test in heroic, decent dps groups should be able to blast him down with no funky strats.
    triggers.push_back(new TriggerNode("poison nova",
        { NextAction("avoid poison nova", ACTION_RAID + 5) }));
    triggers.push_back(new TriggerNode("snake wrap",
        { NextAction("attack snake wrap", ACTION_RAID + 4) }));
    // 2026-09-17：上游这行 TODO 的答案（"burn down boss"）。相关性 55：
    // 压在 `dps assist`(50) 之上、`attack snake wrap`(64) 之下，
    // 所以「有包裹先打包裹」的顺序不变，只是打完/没有包裹时目标回到 boss 而不是小怪。
    triggers.push_back(new TriggerNode("slad'ran focus boss",
        { NextAction("slad'ran focus boss", 55.0f) }));
    // 红蛇优先（相关性 56，压过 focus boss 55、低于 attack snake wrap 64）。
    // 理由与实测数字见 GDActions.cpp 里 SladranFocusViperAction 上方的注释。
    triggers.push_back(new TriggerNode("slad'ran focus viper",
        { NextAction("slad'ran focus viper", 56.0f) }));

    // 2026-09-17：试过「开场就开英勇/嗜血」（挂在共享层的 `combat opening` 触发器、相关性 61），
    // **净负面、已回退**。共享层的 `CombatOpeningTrigger` 保留（可复用、不默认生效）。
    //
    // 靶子指标动得很干净：英勇释放时刻 **41.6 秒 → 7.3 秒**
    //   （改前上游把它卡在 `BoostTrigger` 的 `balance <= 50`，
    //    balance = 队伍等级总和 × 100 / 敌人加权等级总和，
    //    5 人 80 级 = 400、精英 boss 82×3 = 246、每只小怪 81 →
    //    要**场上同时有 7 只以上小怪**才亮，19/19 场都在 41.6 秒才开）。
    //
    // 但结果更差（改前第七刀 19 场 vs 改后 5 场）：
    //   击杀            3/19 (15.8%) → **0/5**
    //   团灭场时长       105.7s → **80.3s（-24%）**
    //   团灭场 boss 残血 20.4% → **31.0%**
    //   打到 boss 的伤害 229.7k → **197.1k（-14%）**
    //
    // **boss 输出速率其实涨了 13%**（2,455/秒 vs 2,173/秒）—— 爆发本身是有效的，
    // 但队伍**早死 24%**，净结果是总伤害更少。原因：提前把 boss 推到 75% 以下，
    // **缠绕者更早开始刷**，单位战斗时间里累积的小怪更多；而英勇只有 40 秒，
    // 后面 60+ 秒没有急速。
    //
    // 教训：「开场 Rush」的前提是**能在爆发窗口内打完**。我们全打 boss 要 47–57 秒、
    // 英勇 40 秒、输出还要分给小怪 —— 前提不满足。
    // 反过来说，上游那个 `balance <= 50` 判据在这个 boss 上**不算错**：
    // 它把爆发留到局面最紧的时候。

    // Gal'darah
    triggers.push_back(new TriggerNode("whirling slash",
        { NextAction("avoid whirling slash", ACTION_RAID + 5) }));

    // Eck the Ferocious (Heroic only)
}

void WotlkDungeonGDStrategy::InitMultipliers(std::vector<Multiplier*> &multipliers)
{
    multipliers.push_back(new SladranMultiplier(botAI));
    multipliers.push_back(new GaldarahMultiplier(botAI));
}
