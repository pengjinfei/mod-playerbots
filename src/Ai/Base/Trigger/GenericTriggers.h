/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_GENERICTRIGGERS_H
#define PLAYERBOTS_GENERICTRIGGERS_H

#include "HealthTriggers.h"
#include "Player.h"
#include "RangeTriggers.h"
#include "RtiTargetValue.h"
#include "Trigger.h"
#include <utility>

class PlayerbotAI;
class Unit;

class StatAvailable : public Trigger
{
public:
    StatAvailable(PlayerbotAI* botAI, int32 amount, std::string const name = "stat available")
        : Trigger(botAI, name), amount(amount) {}

protected:
    int32 amount;
};

class HighManaTrigger : public Trigger
{
public:
    HighManaTrigger(PlayerbotAI* botAI) : Trigger(botAI, "high mana") {}

    bool IsActive() override;
};

class EnoughManaTrigger : public Trigger
{
public:
    EnoughManaTrigger(PlayerbotAI* botAI) : Trigger(botAI, "enough mana") {}

    bool IsActive() override;
};

class AlmostFullManaTrigger : public Trigger
{
public:
    AlmostFullManaTrigger(PlayerbotAI* botAI) : Trigger(botAI, "almost full mana") {}

    bool IsActive() override;
};

class RageAvailable : public StatAvailable
{
public:
    RageAvailable(PlayerbotAI* botAI, int32 amount) : StatAvailable(botAI, amount, "rage available") {}

    bool IsActive() override;
};

class LightRageAvailableTrigger : public RageAvailable
{
public:
    LightRageAvailableTrigger(PlayerbotAI* botAI) : RageAvailable(botAI, 20) {}
};

class MediumRageAvailableTrigger : public RageAvailable
{
public:
    MediumRageAvailableTrigger(PlayerbotAI* botAI) : RageAvailable(botAI, 40) {}
};

class HighRageAvailableTrigger : public RageAvailable
{
public:
    HighRageAvailableTrigger(PlayerbotAI* botAI) : RageAvailable(botAI, 60) {}
};

class EnergyAvailable : public StatAvailable
{
public:
    EnergyAvailable(PlayerbotAI* botAI, int32 amount) : StatAvailable(botAI, amount, "energy available") {}

    bool IsActive() override;
};

class LightEnergyAvailableTrigger : public EnergyAvailable
{
public:
    LightEnergyAvailableTrigger(PlayerbotAI* botAI) : EnergyAvailable(botAI, 20) {}
};

class MediumEnergyAvailableTrigger : public EnergyAvailable
{
public:
    MediumEnergyAvailableTrigger(PlayerbotAI* botAI) : EnergyAvailable(botAI, 40) {}
};

class HighEnergyAvailableTrigger : public EnergyAvailable
{
public:
    HighEnergyAvailableTrigger(PlayerbotAI* botAI) : EnergyAvailable(botAI, 60) {}
};

class ComboPointsAvailableTrigger : public StatAvailable
{
public:
    ComboPointsAvailableTrigger(PlayerbotAI* botAI, int32 amount = 5)
        : StatAvailable(botAI, amount, "combo points available")
    {
    }

    bool IsActive() override;
};

class TargetWithComboPointsLowerHealTrigger : public ComboPointsAvailableTrigger
{
public:
    TargetWithComboPointsLowerHealTrigger(PlayerbotAI* botAI, int32 combo_point = 5, float lifeTime = 8.0f)
        : ComboPointsAvailableTrigger(botAI, combo_point), lifeTime(lifeTime)
    {
    }
    bool IsActive() override;

private:
    float lifeTime;
};

class ComboPointsNotFullTrigger : public StatAvailable
{
public:
    ComboPointsNotFullTrigger(PlayerbotAI* botAI, int32 amount = 5, std::string const name = "combo points not full")
        : StatAvailable(botAI, amount, name)
    {
    }

    bool IsActive() override;
};

class LoseAggroTrigger : public Trigger
{
public:
    LoseAggroTrigger(PlayerbotAI* botAI) : Trigger(botAI, "lose aggro") {}

    bool IsActive() override;
};

class HasAggroTrigger : public Trigger
{
public:
    HasAggroTrigger(PlayerbotAI* botAI) : Trigger(botAI, "have aggro") {}

    bool IsActive() override;
};

class SpellTrigger : public Trigger
{
public:
    SpellTrigger(PlayerbotAI* botAI, std::string const spell, int32 checkInterval = 1)
        : Trigger(botAI, spell, checkInterval), spell(spell)
    {
    }

    std::string const GetTargetName() override { return "current target"; }
    std::string const getName() override { return spell; }
    bool IsActive() override;

protected:
    std::string spell;
};

class SpellCanBeCastTrigger : public SpellTrigger
{
public:
    SpellCanBeCastTrigger(PlayerbotAI* botAI, std::string const spell) : SpellTrigger(botAI, spell) {}

    bool IsActive() override;
};

class SpellNoCooldownTrigger : public SpellTrigger
{
public:
    SpellNoCooldownTrigger(PlayerbotAI* botAI, std::string const spell) : SpellTrigger(botAI, spell) {}

    bool IsActive() override;
};

class SpellCooldownTrigger : public SpellTrigger
{
public:
    SpellCooldownTrigger(PlayerbotAI* botAI, std::string const spell) : SpellTrigger(botAI, spell) {}

    std::string const GetTargetName() override { return "self target"; }
    bool IsActive() override;
};

class InterruptSpellTrigger : public SpellTrigger
{
public:
    InterruptSpellTrigger(PlayerbotAI* botAI, std::string const spell) : SpellTrigger(botAI, spell) {}

    bool IsActive() override;
};

class DeflectSpellTrigger : public SpellTrigger
{
public:
    DeflectSpellTrigger(PlayerbotAI* botAI, std::string const spell) : SpellTrigger(botAI, spell) {}

    bool IsActive() override;
};

class AttackerCountTrigger : public Trigger
{
public:
    AttackerCountTrigger(PlayerbotAI* botAI, int32 amount, float distance = sPlayerbotAIConfig.sightDistance)
        : Trigger(botAI), amount(amount), distance(distance) {}

    bool IsActive() override;
    std::string const getName() override { return "attacker count"; }

protected:
    int32 amount;
    float distance;
};

class HasAttackersTrigger : public AttackerCountTrigger
{
public:
    HasAttackersTrigger(PlayerbotAI* botAI) : AttackerCountTrigger(botAI, 1) {}
};

class MyAttackerCountTrigger : public AttackerCountTrigger
{
public:
    MyAttackerCountTrigger(PlayerbotAI* botAI, int32 amount) : AttackerCountTrigger(botAI, amount) {}

    bool IsActive() override;
    std::string const getName() override { return "my attacker count"; }
};

class BeingAttackedTrigger : public MyAttackerCountTrigger
{
public:
    BeingAttackedTrigger(PlayerbotAI* botAI) : MyAttackerCountTrigger(botAI, 1) {}
    std::string const getName() override { return "being attacked"; }
};

class MediumThreatTrigger : public MyAttackerCountTrigger
{
public:
    MediumThreatTrigger(PlayerbotAI* botAI) : MyAttackerCountTrigger(botAI, 2) {}
    bool IsActive() override;
};

class LowTankThreatTrigger : public Trigger
{
public:
    LowTankThreatTrigger(PlayerbotAI* botAI) : Trigger(botAI, "low tank threat") {}
    bool IsActive() override;
};

// True only for a follower's first combat decision after the main tank has
// established the boss as its victim. Classes can use it for an opening
// threat-transfer spell before their first damaging action.
class OpeningTankThreatTrigger : public Trigger
{
public:
    OpeningTankThreatTrigger(PlayerbotAI* botAI) : Trigger(botAI, "opening tank threat") {}
    bool IsActive() override;
};

class AoeTrigger : public AttackerCountTrigger
{
public:
    AoeTrigger(PlayerbotAI* botAI, int32 amount = 3, float range = 15.0f)
        : AttackerCountTrigger(botAI, amount), range(range) {}

    bool IsActive() override;
    std::string const getName() override { return "aoe"; }

private:
    float range;
};

class NoFoodTrigger : public Trigger
{
public:
    NoFoodTrigger(PlayerbotAI* botAI) : Trigger(botAI, "no food trigger") {}

    bool IsActive() override;
};

class NoDrinkTrigger : public Trigger
{
public:
    NoDrinkTrigger(PlayerbotAI* botAI) : Trigger(botAI, "no drink trigger") {}

    bool IsActive() override;
};

class LightAoeTrigger : public AoeTrigger
{
public:
    LightAoeTrigger(PlayerbotAI* botAI) : AoeTrigger(botAI, 2, 8.0f) {}
};

class MediumAoeTrigger : public AoeTrigger
{
public:
    MediumAoeTrigger(PlayerbotAI* botAI) : AoeTrigger(botAI, 3, 8.0f) {}
};

class HighAoeTrigger : public AoeTrigger
{
public:
    HighAoeTrigger(PlayerbotAI* botAI) : AoeTrigger(botAI, 4, 8.0f) {}
};

class BuffTrigger : public SpellTrigger
{
public:
    BuffTrigger(PlayerbotAI* botAI, std::string const spell, int32 checkInterval = 1,
        bool checkIsOwner = false, bool checkDuration = false, uint32 beforeDuration = 0)
        : SpellTrigger(botAI, spell, checkInterval)
    {
        this->checkIsOwner = checkIsOwner;
        this->checkDuration = checkDuration;
        this->beforeDuration = beforeDuration;
    }

public:
    std::string const GetTargetName() override { return "self target"; }
    bool IsBuffTrigger() override { return true; }
    bool IsActive() override;

protected:
    bool checkIsOwner;
    bool checkDuration;
    uint32 beforeDuration;
};

class BuffOnPartyTrigger : public BuffTrigger
{
public:
    BuffOnPartyTrigger(PlayerbotAI* botAI, std::string const spell, int32 checkInterval = 1)
        : BuffTrigger(botAI, spell, checkInterval) {}

    Value<Unit*>* GetTargetValue() override;
    bool IsActive() override;
    std::string const getName() override { return spell + " on party"; }
};

class ProtectPartyMemberTrigger : public Trigger
{
public:
    ProtectPartyMemberTrigger(PlayerbotAI* botAI) : Trigger(botAI, "protect party member") {}

    std::string const GetTargetName() override { return "party member to protect"; }
    bool IsActive() override;
};

class NoAttackersTrigger : public Trigger
{
public:
    NoAttackersTrigger(PlayerbotAI* botAI) : Trigger(botAI, "no attackers") {}

    bool IsActive() override;
};

class NoTargetTrigger : public Trigger
{
public:
    NoTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "no target") {}

    bool IsActive() override;
};

class InvalidTargetTrigger : public Trigger
{
public:
    InvalidTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "invalid target") {}

    bool IsActive() override;
};

class TargetInSightTrigger : public Trigger
{
public:
    TargetInSightTrigger(PlayerbotAI* botAI) : Trigger(botAI, "target in sight") {}

    bool IsActive() override;
};

class DebuffTrigger : public BuffTrigger
{
public:
    DebuffTrigger(PlayerbotAI* botAI, std::string const spell, int32 checkInterval = 1, bool checkIsOwner = false,
                  float needLifeTime = 8.0f, uint32 beforeDuration = 0)
        : BuffTrigger(botAI, spell, checkInterval, checkIsOwner, false, beforeDuration), needLifeTime(needLifeTime) {}

    std::string const GetTargetName() override { return "current target"; }
    bool IsDebuffTrigger() override { return true; }
    bool IsActive() override;

protected:
    float needLifeTime;
};

class DebuffOnBossTrigger : public DebuffTrigger
{
public:
    DebuffOnBossTrigger(PlayerbotAI* botAI, std::string const spell, int32 checkInterval = 1, bool checkIsOwner = false)
        : DebuffTrigger(botAI, spell, checkInterval, checkIsOwner) {}
    bool IsActive() override;
};

class DebuffOnAttackerTrigger : public DebuffTrigger
{
public:
    DebuffOnAttackerTrigger(PlayerbotAI* botAI, std::string const spell, bool checkIsOwner = true,
                            float needLifeTime = 8.0f)
        : DebuffTrigger(botAI, spell, 1, checkIsOwner, needLifeTime) {}

    Value<Unit*>* GetTargetValue() override;
    std::string const getName() override { return spell + " on attacker"; }
};

class DebuffOnMeleeAttackerTrigger : public DebuffTrigger
{
public:
    DebuffOnMeleeAttackerTrigger(PlayerbotAI* botAI, std::string const spell, bool checkIsOwner = true,
                                 float needLifeTime = 8.0f)
        : DebuffTrigger(botAI, spell, 1, checkIsOwner, needLifeTime) {}

    Value<Unit*>* GetTargetValue() override;
    std::string const getName() override { return spell + " on attacker"; }
};

// 「开场窗口」：战斗中，且本 bot 进入战斗不超过 seconds 秒。
//
// 2026-09-17 新增，用于「开场 rush」类打法（开英勇/嗜血、爆发冷却）。
// **它不在任何策略里默认注册** —— 副本策略要用就自己加一个 TriggerNode，
// 所以对现有 boss 零影响。复用方式：
//     triggers.push_back(new TriggerNode("combat opening",
//         { NextAction("heroism", 61.0f), NextAction("bloodlust", 61.0f) }));
//
// 为什么需要它：上游的 `BoostTrigger` 把英勇/嗜血卡在
// `AI_VALUE(uint8, "balance") <= 50`，而 balance = 队伍等级总和 × 100 / 敌人加权等级总和。
// 5 人 80 级 = 400；英雄斯拉德兰（精英 82 × 3 = 246）+ 每只小怪 81。
// 算下来要**场上同时有 7 只以上小怪**才会亮 —— 实测 19/19 场英勇都在
// **41.6 秒**（最早 35.9 秒）才开，正好是缠绕者刷起来之后。
// 这个判据的本意是「劣势时开爆发」，但对持续刷怪的 boss 等于「等雪球滚大了才开」。
// ⚠ 不要用 `AI_VALUE(time_t, "combat start time")` 来实现这个 ——
// 那个取值**只在 bot 挂了 `"wait for attack"` 策略时才被维护**
// （`PlayerbotAI::ChangeEngineOnCombat()`），我们的 raidtest bot 不挂它，
// 所以它恒为 0。第一版就是这么写的，实测 `T:combat opening` 亮起 **0 次**。
// 这里改成触发器自己记住「进入战斗的那一刻」，不依赖任何别的策略。
class CombatOpeningTrigger : public Trigger
{
public:
    CombatOpeningTrigger(PlayerbotAI* botAI, uint32 seconds = 15)
        : Trigger(botAI, "combat opening"), seconds(seconds), combatStart(0) {}

    bool IsActive() override;

protected:
    uint32 seconds;
    time_t combatStart;
};

class BoostTrigger : public BuffTrigger
{
public:
    BoostTrigger(PlayerbotAI* botAI, std::string const spell, float balance = 50.f)
        : BuffTrigger(botAI, spell, 1), balance(balance) {}

    bool IsActive() override;

protected:
    float balance;
};

class GenericBoostTrigger : public Trigger
{
public:
    GenericBoostTrigger(PlayerbotAI* botAI, float balance = 50.f)
        : Trigger(botAI, "generic boost", 1), balance(balance) {}

    bool IsActive() override;

protected:
    float balance;
};

class HealerShouldAttackTrigger : public Trigger
{
public:
    HealerShouldAttackTrigger(PlayerbotAI* botAI)
        : Trigger(botAI, "healer should attack", 1) {}

    bool IsActive() override;
};

class RandomTrigger : public Trigger
{
public:
    RandomTrigger(PlayerbotAI* botAI, std::string const name, int32 probability = 7);

    bool IsActive() override;

protected:
    int32 probability;
    uint32 lastCheck;
};

class AndTrigger : public Trigger
{
public:
    AndTrigger(PlayerbotAI* botAI, Trigger* ls, Trigger* rs) : Trigger(botAI), ls(ls), rs(rs) {}
    virtual ~AndTrigger()
    {
        delete ls;
        delete rs;
    }

    bool IsActive() override;
    std::string const getName() override;

protected:
    Trigger* ls;
    Trigger* rs;
};

class TwoTriggers : public Trigger
{
public:
    explicit TwoTriggers(PlayerbotAI* botAI, std::string name1 = "", std::string name2 = "") : Trigger(botAI)
    {
        this->name1 = std::move(name1);
        this->name2 = std::move(name2);
    }
    bool IsActive() override;
    std::string const getName() override;

protected:
    std::string name1;
    std::string name2;
};

class SnareTargetTrigger : public DebuffTrigger
{
public:
    SnareTargetTrigger(PlayerbotAI* botAI, std::string const spell) : DebuffTrigger(botAI, spell) {}

    Value<Unit*>* GetTargetValue() override;
    std::string const getName() override { return spell + " on snare target"; }
};

class LowManaTrigger : public Trigger
{
public:
    LowManaTrigger(PlayerbotAI* botAI) : Trigger(botAI, "low mana") {}

    bool IsActive() override;
};

class MediumManaTrigger : public Trigger
{
public:
    MediumManaTrigger(PlayerbotAI* botAI) : Trigger(botAI, "medium mana") {}

    bool IsActive() override;
};

class LowEnergyTrigger : public Trigger
{
public:
    LowEnergyTrigger(PlayerbotAI* botAI, uint8 threshold = 30) : Trigger(botAI, "low energy"), threshold(threshold) {}

    bool IsActive() override;

private:
    uint8 threshold;
};

BEGIN_TRIGGER(PanicTrigger, Trigger) // cppcheck-suppress unknownMacro
std::string const getName() override { return "panic"; }
END_TRIGGER()

BEGIN_TRIGGER(OutNumberedTrigger, Trigger)
std::string const getName() override { return "outnumbered"; }
END_TRIGGER()

class NoPetTrigger : public Trigger
{
public:
    NoPetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "no pet", 5 * 1000) {}

    virtual bool IsActive() override;
};

class HasPetTrigger : public Trigger
{
public:
    HasPetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "has pet", 5 * 1000) {}

    virtual bool IsActive() override;
};

class PetAttackTrigger : public Trigger
{
public:
    PetAttackTrigger(PlayerbotAI* botAI) : Trigger(botAI, "pet attack") {}

    virtual bool IsActive() override;
};

class ItemCountTrigger : public Trigger
{
public:
    ItemCountTrigger(PlayerbotAI* botAI, std::string const item, int32 count, int32 interval = 30 * 1000)
        : Trigger(botAI, item, interval), item(item), count(count) {}

    bool IsActive() override;
    std::string const getName() override { return "item count"; }

protected:
    std::string const item;
    int32 count;
};

class AmmoCountTrigger : public ItemCountTrigger
{
public:
    AmmoCountTrigger(PlayerbotAI* botAI, std::string const item, uint32 count = 1, int32 interval = 30 * 1000)
        : ItemCountTrigger(botAI, item, count, interval) {}
    bool IsActive() override;
};

class HasAuraTrigger : public Trigger
{
public:
    HasAuraTrigger(PlayerbotAI* botAI, std::string const spell, int32 checkInterval = 1)
        : Trigger(botAI, spell, checkInterval) {}

    std::string const GetTargetName() override { return "self target"; }
    bool IsActive() override;
};

class HasAuraStackTrigger : public Trigger
{
public:
    HasAuraStackTrigger(PlayerbotAI* botAI, std::string spell, int stack, int checkInterval = 1)
        : Trigger(botAI, spell, checkInterval), stack(stack) {}

    std::string const GetTargetName() override { return "self target"; }
    bool IsActive() override;

private:
    int stack;
};

class HasNoAuraTrigger : public Trigger
{
public:
    HasNoAuraTrigger(PlayerbotAI* botAI, std::string const spell) : Trigger(botAI, spell) {}

    std::string const GetTargetName() override { return "self target"; }
    bool IsActive() override;
};

class TimerTrigger : public Trigger
{
public:
    TimerTrigger(PlayerbotAI* botAI) : Trigger(botAI, "timer"), lastCheck(0) {}

    bool IsActive() override;

private:
    time_t lastCheck;
};

class TimerBGTrigger : public Trigger
{
public:
    TimerBGTrigger(PlayerbotAI* botAI) : Trigger(botAI, "timer bg"), lastCheck(0) {}

    bool IsActive() override;

private:
    time_t lastCheck;
};

class TankAssistTrigger : public NoAttackersTrigger
{
public:
    TankAssistTrigger(PlayerbotAI* botAI) : NoAttackersTrigger(botAI) {}

    bool IsActive() override;
};

class IsBehindTargetTrigger : public Trigger
{
public:
    IsBehindTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "behind target") {}

    bool IsActive() override;
};

class IsNotBehindTargetTrigger : public Trigger
{
public:
    IsNotBehindTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "is not behind target") {}

    bool IsActive() override;
};

class IsNotFacingTargetTrigger : public Trigger
{
public:
    IsNotFacingTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "not facing target") {}

    bool IsActive() override;
};

class HasCcTargetTrigger : public Trigger
{
public:
    HasCcTargetTrigger(PlayerbotAI* botAI, std::string const name) : Trigger(botAI, name) {}

    bool IsActive() override;

protected:
    bool IsCcTargetFree(Unit* ccTarget, Unit* rtiCcTarget);
};

class NoMovementTrigger : public Trigger
{
public:
    NoMovementTrigger(PlayerbotAI* botAI, std::string const name) : Trigger(botAI, name) {}

    bool IsActive() override;
};

class NoPossibleTargetsTrigger : public Trigger
{
public:
    NoPossibleTargetsTrigger(PlayerbotAI* botAI) : Trigger(botAI, "no possible targets") {}

    bool IsActive() override;
};

class NotDpsTargetActiveTrigger : public Trigger
{
public:
    NotDpsTargetActiveTrigger(PlayerbotAI* botAI) : Trigger(botAI, "not dps target active") {}

    bool IsActive() override;
};

class NotDpsAoeTargetActiveTrigger : public Trigger
{
public:
    NotDpsAoeTargetActiveTrigger(PlayerbotAI* botAI) : Trigger(botAI, "not dps aoe target active") {}

    bool IsActive() override;
};

class PossibleAddsTrigger : public Trigger
{
public:
    PossibleAddsTrigger(PlayerbotAI* botAI) : Trigger(botAI, "possible adds") {}

    bool IsActive() override;
};

class LossOfControlTrigger : public Trigger
{
public:
    LossOfControlTrigger(PlayerbotAI* botAI) : Trigger(botAI, "loss of control", 1) {}

    bool IsActive() override;
};

class FearCharmSleepTrigger : public Trigger
{
public:
    FearCharmSleepTrigger(PlayerbotAI* botAI) : Trigger(botAI, "fear charm sleep", 1) {}

    bool IsActive() override;
};

class FearSleepSapTrigger : public Trigger
{
public:
    FearSleepSapTrigger(PlayerbotAI* botAI) : Trigger(botAI, "fear sleep sap", 1) {}

    bool IsActive() override;
};

class PoisonDiseaseBleedTrigger : public Trigger
{
public:
    PoisonDiseaseBleedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "poison disease bleed", 1) {}

    bool IsActive() override;
};

class MovementImpairedTrigger : public Trigger
{
public:
    MovementImpairedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "movement impaired", 1) {}

    bool IsActive() override;
};

class IsSwimmingTrigger : public Trigger
{
public:
    IsSwimmingTrigger(PlayerbotAI* botAI) : Trigger(botAI, "swimming") {}

    bool IsActive() override;
};

class HasNearestAddsTrigger : public Trigger
{
public:
    HasNearestAddsTrigger(PlayerbotAI* botAI) : Trigger(botAI, "has nearest adds") {}

    bool IsActive() override;
};

class HasItemForSpellTrigger : public Trigger
{
public:
    HasItemForSpellTrigger(PlayerbotAI* botAI, std::string const spell) : Trigger(botAI, spell) {}

    bool IsActive() override;
};

class TargetChangedTrigger : public Trigger
{
public:
    TargetChangedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "target changed") {}

    bool IsActive() override;
};

class InterruptEnemyHealerTrigger : public SpellTrigger
{
public:
    InterruptEnemyHealerTrigger(PlayerbotAI* botAI, std::string const spell) : SpellTrigger(botAI, spell) {}

    Value<Unit*>* GetTargetValue() override;
    std::string const getName() override { return spell + " on enemy healer"; }
};

class RandomBotUpdateTrigger : public RandomTrigger
{
public:
    RandomBotUpdateTrigger(PlayerbotAI* botAI) : RandomTrigger(botAI, "random bot update", 30 * 1000) {}

    bool IsActive() override;
};

class NoNonBotPlayersAroundTrigger : public Trigger
{
public:
    NoNonBotPlayersAroundTrigger(PlayerbotAI* botAI) : Trigger(botAI, "no non bot players around", 10 * 1000) {}

    bool IsActive() override;
};

class NewPlayerNearbyTrigger : public Trigger
{
public:
    NewPlayerNearbyTrigger(PlayerbotAI* botAI) : Trigger(botAI, "new player nearby", 10 * 1000) {}

    bool IsActive() override;
};

class CollisionTrigger : public Trigger
{
public:
    CollisionTrigger(PlayerbotAI* botAI) : Trigger(botAI, "collision", 5 * 1000) {}

    bool IsActive() override;
};

class StayTimeTrigger : public Trigger
{
public:
    StayTimeTrigger(PlayerbotAI* botAI, uint32 delay, std::string const name)
        : Trigger(botAI, name, 5 * 1000), delay(delay) {}

    bool IsActive() override;

private:
    uint32 delay;
};

class SitTrigger : public StayTimeTrigger
{
public:
    SitTrigger(PlayerbotAI* botAI) : StayTimeTrigger(botAI, sPlayerbotAIConfig.sitDelay, "sit") {}
};

class ReturnToStayPositionTrigger : public Trigger
{
public:
    ReturnToStayPositionTrigger(PlayerbotAI* botAI) : Trigger(botAI, "return to stay position", 2) {}

    virtual bool IsActive() override;
};

class ReturnTrigger : public StayTimeTrigger
{
public:
    ReturnTrigger(PlayerbotAI* botAI) : StayTimeTrigger(botAI, sPlayerbotAIConfig.returnDelay, "return") {}
};

class GiveItemTrigger : public Trigger
{
public:
    GiveItemTrigger(PlayerbotAI* botAI, std::string const name, std::string const item)
        : Trigger(botAI, name, 2 * 1000), item(item) {}

    bool IsActive() override;

protected:
    std::string const item;
};

class GiveFoodTrigger : public GiveItemTrigger
{
public:
    GiveFoodTrigger(PlayerbotAI* botAI) : GiveItemTrigger(botAI, "give food", "conjured food") {}

    bool IsActive() override;
};

class GiveWaterTrigger : public GiveItemTrigger
{
public:
    GiveWaterTrigger(PlayerbotAI* botAI) : GiveItemTrigger(botAI, "give water", "conjured water") {}

    bool IsActive() override;
};

class IsMountedTrigger : public Trigger
{
public:
    IsMountedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mounted", 1) {}

    bool IsActive() override;
};

class CorpseNearTrigger : public Trigger
{
public:
    CorpseNearTrigger(PlayerbotAI* botAI) : Trigger(botAI, "corpse near", 1 * 1000) {}

    bool IsActive() override;
};

class IsFallingTrigger : public Trigger
{
public:
    IsFallingTrigger(PlayerbotAI* botAI) : Trigger(botAI, "falling", 10 * 1000) {}

    bool IsActive() override;
};

class IsFallingFarTrigger : public Trigger
{
public:
    IsFallingFarTrigger(PlayerbotAI* botAI) : Trigger(botAI, "falling far", 10 * 1000) {}

    bool IsActive() override;
};

class HasAreaDebuffTrigger : public Trigger
{
public:
    HasAreaDebuffTrigger(PlayerbotAI* botAI) : Trigger(botAI, "have area debuff") {}

    bool IsActive() override;
};

class BuffOnMainTankTrigger : public BuffTrigger
{
public:
    BuffOnMainTankTrigger(PlayerbotAI* botAI, std::string spell, bool checkIsOwner = false, int checkInterval = 1)
        : BuffTrigger(botAI, spell, checkInterval, checkIsOwner) {}

public:
    virtual Value<Unit*>* GetTargetValue();
};

class SelfResurrectTrigger : public Trigger
{
public:
    SelfResurrectTrigger(PlayerbotAI* botAI) : Trigger(botAI, "can self resurrect") {}

    bool IsActive() override { return !bot->IsAlive() && bot->GetUInt32Value(PLAYER_SELF_RES_SPELL); }
};

class NewPetTrigger : public Trigger
{
public:
    NewPetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "new pet"), lastPetGuid(ObjectGuid::Empty), triggered(false) {}

    bool IsActive() override;

private:
    ObjectGuid lastPetGuid;
    bool triggered;
};

class ForceRebuffPendingTrigger : public Trigger
{
public:
    ForceRebuffPendingTrigger(PlayerbotAI* botAI) : Trigger(botAI, "force rebuff pending") {}

    bool IsActive() override;
};

// ---- 清怪控制链（TrashCcPullStrategy 用）----
class TrashCcMarkTrigger : public Trigger
{
public:
    TrashCcMarkTrigger(PlayerbotAI* ai) : Trigger(ai, "trash cc mark") {}
    bool IsActive() override { return TrashCcMarkNeeded(botAI, bot); }
};

class TrashCcCastTrigger : public Trigger
{
public:
    TrashCcCastTrigger(PlayerbotAI* ai, std::string const name, uint8 casterClass)
        : Trigger(ai, name), casterClass(casterClass) {}
    bool IsActive() override;

private:
    uint8 casterClass;
};

#endif
