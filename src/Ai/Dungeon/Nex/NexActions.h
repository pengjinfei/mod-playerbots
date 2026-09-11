/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NEXACTIONS_H
#define PLAYERBOTS_NEXACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "GenericSpellActions.h"
#include "NexTriggers.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

#include <map>

class MoveFromWhirlwindAction : public MovementAction
{
public:
    MoveFromWhirlwindAction(PlayerbotAI* ai) : MovementAction(ai, "move from whirlwind") {}
    bool Execute(Event event) override;
};

class FirebombSpreadAction : public MovementAction
{
public:
    FirebombSpreadAction(PlayerbotAI* ai) : MovementAction(ai, "firebomb spread") {}
    bool Execute(Event event) override;
};

class TelestraSplitTargetAction : public AttackAction
{
public:
    TelestraSplitTargetAction(PlayerbotAI* ai) : AttackAction(ai, "telestra split target") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class ChaoticRiftTargetAction : public AttackAction
{
public:
    ChaoticRiftTargetAction(PlayerbotAI* ai) : AttackAction(ai, "chaotic rift target") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

// 继承 CastSpellAction（而不是裸 Action）是刻意的：上游的 isPossible() 会走
// botAI->CanCastSpell()，它会在已有读条时返回 false。run374 里自建的裸 Action 版变形术
// 因为绕过这条保护，每 1.4 秒把自己的读条顶掉一次、78 次施放零成功。
// 妖术与变形术都有读条（实测 1082-1407 ms），这条保护是必需的。
class TrashHealerCcAction : public CastSpellAction
{
public:
    TrashHealerCcAction(PlayerbotAI* ai, std::string const spell, uint8 casterClass, bool farthest,
                        std::string const name)
        : CastSpellAction(ai, spell), casterClass(casterClass), farthest(farthest), name(name) {}

    // 目标不走共享的 "cc target"：那条取值里「不要 CC 已经在 AoE 团里的怪」对普通小怪是
    // 正确默认，但守卫组 4 只是叠在一起的，这条会把治疗全部排除掉。让 AoE 给控制让路这件事
    // 由 CrowdControlProtectionMultiplier 负责，不是靠放弃控制来回避。
    Unit* GetTarget() override;
    bool Execute(Event event) override;
    std::string const getName() override { return name; }
    ActionThreatType getThreatType() override { return ActionThreatType::None; }

protected:
    // 控制被打破后的重试间隔。run390 实测妖术只撑 1-4 秒，不设这个间隔的话
    // 「掉了就补」会把整段清怪的 GCD 全花在控制上（run374 的空转是另一个原因，
    // 但后果一样）。同一只怪在这个窗口内不再尝试。
    static constexpr uint32 kRetryCooldownMs = 10000;

    uint8 casterClass;
    bool farthest;
    std::string name;
    std::map<ObjectGuid, uint32> lastCast;
};

class TrashHealerHexAction : public TrashHealerCcAction
{
public:
    TrashHealerHexAction(PlayerbotAI* ai)
        : TrashHealerCcAction(ai, "hex", CLASS_SHAMAN, false, "trash healer hex") {}
};

class TrashHealerPolymorphAction : public TrashHealerCcAction
{
public:
    TrashHealerPolymorphAction(PlayerbotAI* ai)
        : TrashHealerCcAction(ai, "polymorph", CLASS_MAGE, true, "trash healer polymorph") {}
};

class DodgeSpikesAction : public MovementAction
{
public:
    DodgeSpikesAction(PlayerbotAI* ai) : MovementAction(ai, "dodge spikes") {}
    bool Execute(Event event) override;
    bool isUseful() override;
};

class IntenseColdJumpAction : public MovementAction
{
public:
    IntenseColdJumpAction(PlayerbotAI* ai) : MovementAction(ai, "intense cold jump") {}
    bool Execute(Event event) override;
};

#endif
