/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_CROWDCONTROLPROTECTIONMULTIPLIER_H
#define PLAYERBOTS_CROWDCONTROLPROTECTIONMULTIPLIER_H

#include "Multiplier.h"

class Action;
class PlayerbotAI;
class Unit;

// 别用自家的 AoE 打破自家的控制。
//
// 上游已有的是「选谁来 CC」（CcTargetValue），而且它里面那条「不要 CC 已经在 AoE 团里的怪」
// 只作用于兜底路径 —— 被团队标记指定的目标会在该排除之前就命中。也就是说一旦开始用标记
// 分配控制，就没有任何东西阻止队伍的 AoE 砸到已经被控住的怪；唯一名字相近的
// AvoidAoeStrategy 讲的是「bot 躲开敌方 AoE」，它的 InitMultipliers 是空实现。
//
// 判据完全由法术数据驱动，不维护任何动作名清单（这个代码库里 AoE 动作没有公共基类，
// 由 "light aoe" / "medium aoe" 这类触发器驱动、各职业各写各的）：
//   * 动作是 CastSpellAction 派生，且其法术 SpellInfo::IsTargetingArea() 为真、非正面法术；
//   * 附近有敌对单位带着「受击/受伤即移除」的光环（AURA_INTERRUPT_FLAG_NOT_VICTIM =
//     HITBYSPELL | TAKE_DAMAGE | DIRECT_DAMAGE），该光环由友方施加，**且它是真正让怪
//     脱离战斗的那几类**（confuse / fear / stun / pacify-silence / transform）——
//     定身与减速不算，被定住的怪照样在打人，护着它等于白亏输出；
//   * 该单位落在这个 AoE 的半径内（同时按自身与当前目标两个圆心检查）。
// 满足则返回 0，让位给单体输出。
//
// 注意两点取舍：
//   * 昏迷类控制（制裁之锤、肾击）不吃伤害就不会掉，它们的光环没有那组中断标志位，
//     所以不会被这条误伤。
//   * 只判「受伤即掉」是不够的：冰霜新星(42917) 的定身也带这个标志，实测会把全队 AoE
//     压死整整 8 秒（run385，奥莫洛克那组从 6/8 掉到 1/4），所以还要再过一道
//     「是否让怪脱离战斗」的筛。
//   * 坦克的 AoE 仇恨技（奉献、正义之锤）同样会被压住。这是真人也会做的取舍：
//     控住一只的时候用单体仇恨，不要为了铺仇恨把控制打破。
class CrowdControlProtectionMultiplier : public Multiplier
{
public:
    CrowdControlProtectionMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "crowd control protection") {}

    float GetValue(Action* action) override;

private:
    // 该敌对单位身上是否有「友方施加、且会被伤害打破」的光环。
    bool HasBreakableFriendlyCc(Unit* unit) const;
};

#endif
