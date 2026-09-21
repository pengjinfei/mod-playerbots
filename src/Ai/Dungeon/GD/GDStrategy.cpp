/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GDStrategy.h"
#include "GDMultipliers.h"

void WotlkDungeonGDStrategy::InitTriggers(std::vector<TriggerNode*> &triggers)
{
    // 前置清怪的控制链（共享实现在 TrashCcPullStrategy，MarkRtiStrategy.h）：
    // 坦克按队伍图标指派（骷髅=先杀、月亮=法师羊、方块=萨满妖术、十字=盗贼闷棍）→ 闷棍先、
    // 羊/妖术后 → 编排层等控制落地再开怪（场景 conf 的 PrerequisiteCcWaitSeconds）→
    // 有控制就不放 AoE（全局乘子）→ 单体按序、被控的最后杀。
    // 详见 GDStrategy.h 顶部注释：为什么要接、以及本副本的包几何（西侧三只）。
    TrashCcPullStrategy::InitTriggers(triggers);

    // Moorabi：29819 的 40546 是攻击者承受 22858 反伤的短窗口。仅刺杀盗贼暂停，
    // 不能用全队停手替代（旧样本已否掉该高代价形状）。
    triggers.push_back(new TriggerNode("moorabi lancer retaliation",
        { NextAction("moorabi lancer retaliation wait", ACTION_RAID + 10) }));

    // Drakkari Colossus

    // Slad'ran
    // TODO: Might need to add target priority for heroic on the snakes or to burn down boss.
    // Will re-test in heroic, decent dps groups should be able to blast him down with no funky strats.
    triggers.push_back(new TriggerNode("poison nova",
        { NextAction("avoid poison nova", ACTION_RAID + 5) }));
    triggers.push_back(new TriggerNode("snake wrap",
        { NextAction("attack snake wrap", ACTION_RAID + 4) }));
    triggers.push_back(new TriggerNode("slad'ran focus boss",
        { NextAction("slad'ran focus boss", 55.0f) }));
    triggers.push_back(new TriggerNode("slad'ran stack on tank",
        { NextAction("slad'ran stack on tank", ACTION_RAID + 1) }));
    triggers.push_back(new TriggerNode("slad'ran tank hold",
        { NextAction("slad'ran tank hold", ACTION_RAID + 1) }));

    // Gal'darah
    triggers.push_back(new TriggerNode("whirling slash",
        { NextAction("avoid whirling slash", ACTION_RAID + 5) }));

    // Eck the Ferocious (Heroic only)
}

void WotlkDungeonGDStrategy::InitMultipliers(std::vector<Multiplier*> &multipliers)
{
    multipliers.push_back(new SladranMultiplier(botAI));
    multipliers.push_back(new GaldarahMultiplier(botAI));
    multipliers.push_back(new MoorabiLancerRetaliationMultiplier(botAI));
}
