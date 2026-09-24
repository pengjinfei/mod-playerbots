/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ANACTIONS_H
#define PLAYERBOTS_ANACTIONS_H

#include "ANTriggers.h"
#include "Action.h"
#include "AttackAction.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

class AttackWebWrapAction : public AttackAction
{
public:
    AttackWebWrapAction(PlayerbotAI* ai) : AttackAction(ai, "attack web wrap") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class WatchersTargetAction : public AttackAction
{
public:
    WatchersTargetAction(PlayerbotAI* ai) : AttackAction(ai, "krik'thir priority") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class AnubarakDodgePoundAction : public AttackAction
{
public:
    AnubarakDodgePoundAction(PlayerbotAI* ai) : AttackAction(ai, "anub'arak dodge pound") {}
    bool Execute(Event event) override;
    bool isUseful() override;
    MovementIntent GetMovementIntent() const override { return MovementIntent::SURVIVAL; }
};

// DPS 把最近的、已进入战场的毒疗者写进 "prioritized targets"，没有时清掉（见 AnubarakVenomancerFocusTrigger）。
class AnubarakFocusVenomancerAction : public Action
{
public:
    AnubarakFocusVenomancerAction(PlayerbotAI* ai) : Action(ai, "anub'arak focus venomancer") {}
    bool Execute(Event event) override;
};

// 践踏读条时给主坦预盾 / 接一记治疗（治疗专用，见 AnubarakPoundHealerTrigger）。
class AnubarakPoundShieldTankAction : public CastSpellAction
{
public:
    AnubarakPoundShieldTankAction(PlayerbotAI* ai) : CastSpellAction(ai, "power word: shield") {}
    std::string const getName() override { return "anub'arak pound shield tank"; }
    std::string const GetTargetName() override { return "main tank"; }
};

class AnubarakPoundHealTankAction : public CastSpellAction
{
public:
    AnubarakPoundHealTankAction(PlayerbotAI* ai) : CastSpellAction(ai, "penance") {}
    std::string const getName() override { return "anub'arak pound heal tank"; }
    std::string const GetTargetName() override { return "main tank"; }
};

// 治疗没蓝时的补位治疗：用治疗波（3 秒读条，但每点法力的治疗量最高，比次级治疗波划算）。
class AnubarakOffhealAction : public CastSpellAction
{
public:
    AnubarakOffhealAction(PlayerbotAI* ai) : CastSpellAction(ai, "healing wave") {}
    std::string const getName() override { return "anub'arak offheal"; }
    std::string const GetTargetName() override { return "party member to heal"; }
};

float AnubarakOffAxisAngle(Unit* boss, Player* bot);

// 近战默认绕背：真人近战本来就站 boss 背后，践踏锥（正面 120°/15 码）根本扫不到。
// run 513/514 实测盗贼 35% 的时间站在锥内、中位夹角只有 50°，两次 32k 一击秒杀都发生在锥内。
// 「看到读条再躲」（dodge pound）是被动闪避，3.2 秒内从 50° 跑出锥经常来不及。
class AnubarakMeleeBehindAction : public MovementAction
{
public:
    AnubarakMeleeBehindAction(PlayerbotAI* ai) : MovementAction(ai, "anub'arak melee behind") {}
    bool Execute(Event event) override;
    MovementIntent GetMovementIntent() const override { return MovementIntent::TACTICAL; }
};

// 西沿护栏：x 低于安全线就沿 +x 挪回 kArenaGuardX。保命动作：掉出平台等于整场缺席。
class AnubarakRimGuardAction : public MovementAction
{
public:
    AnubarakRimGuardAction(PlayerbotAI* ai) : MovementAction(ai, "anub'arak rim guard") {}
    bool Execute(Event event) override;
    MovementIntent GetMovementIntent() const override { return MovementIntent::SURVIVAL; }
};

// 远程保距：沿 boss->自己 方向退到 kRangedKeepTarget，目标点被西沿安全线夹住。
class HadronoxTankLeaveAcidAction : public MovementAction
{
public:
    HadronoxTankLeaveAcidAction(PlayerbotAI* ai) : MovementAction(ai, "hadronox tank leave acid") {}
    bool Execute(Event event) override;
};

class AnubarakKeepRangeAction : public MovementAction
{
public:
    AnubarakKeepRangeAction(PlayerbotAI* ai) : MovementAction(ai, "anub'arak keep range") {}
    bool Execute(Event event) override;
    // 战术型（默认）：若标成 POSITIONING，下一次施法就会把这次 1–2 秒的后退打断，永远退不到位。

    // 供躲践踏复用：在以 boss 为圆心、radius 为半径的圆上，从"boss->bot"径向开始每 15° 左右交替旋转，
    // 取第一个在平台安全圆内、x >= kArenaGuardX、且 vmap 有同层地面的点（z 一并给出）。找不到返回 false。
    static bool PickPointAwayFromBoss(Unit* boss, Player* bot, float radius, float& x, float& y, float& z,
                                      bool keepDistance = false);
};

// 躲开穿刺尖刺：尖刺生成后 4 秒才落伤害、半径只有 4 码，走开两步就行。
class AnubarakDodgeImpaleAction : public MovementAction
{
public:
    AnubarakDodgeImpaleAction(PlayerbotAI* ai) : MovementAction(ai, "anub'arak dodge impale") {}
    bool Execute(Event event) override;
    MovementIntent GetMovementIntent() const override { return MovementIntent::SURVIVAL; }
};

#endif
