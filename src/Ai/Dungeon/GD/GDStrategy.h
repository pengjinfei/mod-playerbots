/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_GDSTRATEGY_H
#define PLAYERBOTS_GDSTRATEGY_H

#include "MarkRtiStrategy.h"
#include "RtiTargetValue.h"
#include "Multiplier.h"
#include "Strategy.h"

// 继承 TrashCcPullStrategy：清怪控制链（指派/上控/不放 AoE/按序击杀）是共享能力，这里挂上。
//
// 莫拉比前置清怪为什么要用它（2026-09-18 晚第三轮量化后定为首选）：
//   把前置窗口内玩家承伤按来源拆开后，单一大头是 29829 Earthshaker 打坦克 494,710
//   （占坦克承伤 57%），但它**不是杠杆**——它的最大生命是其余四只的 1.6 倍
//   （105,894 vs 65,165），打得多首先是因为活得久；按 kill/fail 分组逐来源做
//   Mann-Whitney U **没有一项显著**（29829 p=0.53、反伤 p=0.66、Lancer 普攻 p=0.06），
//   连队伍总承伤都不可区分（kill 中位 90,788 / fail 104,022，p=0.85）。
//   连续三轮换靶子（Lancer 目标选择 → Lancer 反伤 → 29829 承伤）都错在同一个地方：
//   **从承伤排行里挑最大项**。控制链改的是「同时接敌数量与焦点」，是过程而不是某只怪。
//
// 本副本的包几何（`raidtest los` 实测，准备点 1772,875,124.44）：
//   以最近的 127067 Earthshaker 为拉怪目标时，pack = **西侧三只**
//   （127067/127113/127068，彼此 4–6 码），三只对准备点全部 los=true、22–26 码（在 30 码
//   控制射程内、在 21 码仇恨圈外）。
//   东侧两只（127062 FireWeaver 38.0 码、127051 Lancer 31.9 码）**既超出 15 码 pack 半径、
//   也超出 30 码控制射程，且 los=false**——它们不在控制链的作用域里，仍然靠坦克抓 + 正常清。
// 不登记 TrashCcRegisterHealerEntries：古达克前置没有治疗型小怪
//   （29874 Inciter 是近战 Strike，29822 FireWeaver 是唯一有法力的），
//   按共享层默认的「有法力 > 其它」排，FireWeaver 会被优先控。
class WotlkDungeonGDStrategy : public TrashCcPullStrategy
{
public:
    WotlkDungeonGDStrategy(PlayerbotAI* ai) : TrashCcPullStrategy(ai)
    {
        // 莫拉比西侧包的安全控制点到十字目标约 27 码且初始无 LOS；盗贼必须潜行贴到
        // 11 码，途中普通接近会先使本人成为战斗状态，闷棍动作随后被硬拒绝（run673）。
        // 只在古达克禁用 Sap，让两个远程名额给羊/妖术；共享链和其它副本不受影响。
        TrashCcRegisterDisabledClasses(604, { CLASS_ROGUE });
    }
    virtual std::string const getName() override { return "gundrak"; }
    virtual void InitTriggers(std::vector<TriggerNode*> &triggers) override;
    virtual void InitMultipliers(std::vector<Multiplier*> &multipliers) override;
};

#endif
