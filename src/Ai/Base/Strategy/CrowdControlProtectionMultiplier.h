/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_CROWDCONTROLPROTECTIONMULTIPLIER_H
#define PLAYERBOTS_CROWDCONTROLPROTECTIONMULTIPLIER_H

#include "Multiplier.h"

#include <string>

class Action;
class PlayerbotAI;
class SpellInfo;
class Unit;

// 有控制在，全队就不放 AoE。
//
// 这是清怪控制链（docs/testing/TRASH-CC-PULL-DESIGN.md）的第 4 步，真人打法里的一句话：
// 「只要还有一只被控住，全队一律不放 AoE，单体一个一个杀。」
//
// 上一版（run385–392）的形状是几何判据：只在「AoE 半径盖到被控的怪」时让路。它被
// run392/attempt3 决定性地推翻——羊上身 2 毫秒就被**已经铺在地上**的奉献打掉：乘子只能压
// 「要不要发起新的 AoE 施法」，压不住存量地面 AoE。所以判据改成本次拉怪的全局状态：
//   * 动作是 CastSpellAction 派生、其法术为负面，且是多目标法术——
//     SpellInfo::IsTargetingArea() / IsAffectingArea()（烈焰风暴、刀扇、新星，以及奉献、暴风雪这类
//     PERSISTENT_AREA_AURA 的地面 AoE）、任一效果 ChainTarget > 1（闪电链、正义之锤、顺劈、
//     复仇者之盾——链式跳转同样会砸到被控的怪），或名单里的间接 AoE（活体炸弹、剑刃乱舞、
//     杀戮盛宴、熔岩图腾等：施放本身是单体/自身增益，伤害之后才落到周围）；
//   * 附近有敌对单位满足二者之一：
//       - 被队伍的控制图标（月亮/方块/十字）钉住——即使控制此刻掉了、正在等重新上控，
//         也不能在它身边铺 AoE，否则新控制一落地就会像 run392 那样被存量 AoE 打掉；
//       - 身上有友方施加的「让它脱离战斗」的控制光环（变形/妖术/致盲/恐惧/闷棍）。
//         不要求 AURA_INTERRUPT_FLAG_TAKE_DAMAGE：妖术(51514) 没有这一位却实测 1–4 秒就掉
//         （台账 2026-09-11），核心 HasBreakableByDamageCrowdControlAura() 对它会漏判。
//         定身与减速不算（被定住的怪照样在打人，run385 冰霜新星压死全队 AoE 8.3 秒）；
//         普通昏迷（制裁之锤、肾击）不算，只认 MECHANIC_SAPPED 的闷棍。
// 满足则返回 0，让位给单体输出。坦克的 AoE 仇恨技（奉献、正义之锤）同样被压住——
// 控住一只的时候用单体仇恨，不要为了铺仇恨把控制打破。
class CrowdControlProtectionMultiplier : public Multiplier
{
public:
    CrowdControlProtectionMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "crowd control protection") {}

    float GetValue(Action* action) override;

private:
    bool IsMultiTargetSpell(SpellInfo const* spellInfo, std::string const& actionSpell) const;
    bool IsIndirectAoe(std::string const& actionSpell) const;
    bool HasFriendlyCrowdControl(Unit* unit) const;
    bool IsCrowdControlIconTarget(Unit* unit) const;
};

#endif
