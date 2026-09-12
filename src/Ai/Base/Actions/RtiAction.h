/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_RTIACTION_H
#define PLAYERBOTS_RTIACTION_H

#include "Action.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "RtiTargetValue.h"

class PlayerbotAI;

class RtiAction : public Action
{
public:
    RtiAction(PlayerbotAI* botAI) : Action(botAI, "rti") {}

    bool Execute(Event event) override;

private:
    void AppendRti(std::ostringstream& out, std::string const type);
};

class MarkRtiAction : public Action
{
public:
    MarkRtiAction(PlayerbotAI* botAI) : Action(botAI, "mark rti") {}

    bool Execute(Event event) override;
};

// ---- 清怪控制链（docs/testing/TRASH-CC-PULL-DESIGN.md）----

// 坦克指派：开怪前把骷髅/月亮/方块/十字钉到成组的怪身上；战斗中骷髅目标死后
// 把骷髅挪到下一只（先未被控的，都控着就按 十字→方块→月亮 放出来打，被控的最后杀）。
class TrashCcMarkAction : public Action
{
public:
    TrashCcMarkAction(PlayerbotAI* ai) : Action(ai, "trash cc mark") {}
    bool Execute(Event event) override;
    bool isUseful() override { return TrashCcMarkNeeded(botAI, bot); }

private:
    bool AssignPrePull(Group* group);
    bool AdvanceKillOrder(Group* group);
    void SetIcon(Group* group, uint8 icon, Unit* target);
};

// 继承 CastSpellAction（而不是裸 Action）是刻意的：上游的 isPossible() 会走
// botAI->CanCastSpell()，它会在已有读条时返回 false。run374 里自建的裸 Action 版变形术
// 因为绕过这条保护，每 1.4 秒把自己的读条顶掉一次、78 次施放零成功。
// 妖术与变形术都有读条（实测 1082-1407 ms），这条保护是必需的。
// 目标不走共享的 "cc target"（它基于仇恨表，脱战为空），而是直接读队伍图标：
// 开怪前首次上控与战斗中重新上控是同一个动作。
class TrashCcCastAction : public CastSpellAction
{
public:
    TrashCcCastAction(PlayerbotAI* ai, TrashCcRole const& role, std::string const name)
        : CastSpellAction(ai, role.spell), role(role), name(name) {}

    Unit* GetTarget() override { return TrashCcCastTarget(botAI, bot, role); }
    bool isUseful() override { return GetTarget() != nullptr; }
    std::string const getName() override { return name; }
    ActionThreatType getThreatType() override { return ActionThreatType::None; }

protected:
    TrashCcRole const& role;
    std::string name;
};

class TrashCcPolymorphAction : public TrashCcCastAction
{
public:
    TrashCcPolymorphAction(PlayerbotAI* ai)
        : TrashCcCastAction(ai, *TrashCcRoleForClass(CLASS_MAGE), "trash cc polymorph") {}
};

class TrashCcHexAction : public TrashCcCastAction
{
public:
    TrashCcHexAction(PlayerbotAI* ai)
        : TrashCcCastAction(ai, *TrashCcRoleForClass(CLASS_SHAMAN), "trash cc hex") {}
};

// 闷棍要求潜行、10 码内、目标未进战斗，所以是一个带走位的动作：
// 先潜行，再贴近，最后出手。战斗中不可用（判据里已排除）。
class TrashCcSapAction : public MovementAction
{
public:
    TrashCcSapAction(PlayerbotAI* ai) : MovementAction(ai, "trash cc sap") {}
    bool Execute(Event event) override;
    bool isUseful() override;
    ActionThreatType getThreatType() override { return ActionThreatType::None; }
};

#endif
