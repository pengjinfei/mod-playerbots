/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_HEALTHTRIGGERS_H
#define PLAYERBOTS_HEALTHTRIGGERS_H

#include "PlayerbotAIConfig.h"
#include "Trigger.h"
#include <stdexcept>

class PlayerbotAI;

class ValueInRangeTrigger : public Trigger
{
public:
    ValueInRangeTrigger(PlayerbotAI* botAI, std::string const name, float maxValue, float minValue)
        : Trigger(botAI, name), maxValue(maxValue), minValue(minValue)
    {
    }

    virtual float GetValue() = 0;
    bool IsActive() override
    {
        float value = GetValue();
        return value < maxValue && value >= minValue;
    }

protected:
    float maxValue, minValue;
};

class HealthInRangeTrigger : public ValueInRangeTrigger
{
public:
    HealthInRangeTrigger(PlayerbotAI* botAI, std::string const name, float maxValue, float minValue = 0)
        : ValueInRangeTrigger(botAI, name, maxValue, minValue)
    {
    }

    bool IsActive() override;
    float GetValue() override;
};

class LowHealthTrigger : public HealthInRangeTrigger
{
public:
    LowHealthTrigger(PlayerbotAI* botAI, std::string const name = "low health",
                     float value = sPlayerbotAIConfig.lowHealth, float minValue = 0)
        : HealthInRangeTrigger(botAI, name, value, minValue)
    {
    }

    std::string const GetTargetName() override { return "self target"; }
};

class CriticalHealthTrigger : public LowHealthTrigger
{
public:
    CriticalHealthTrigger(PlayerbotAI* botAI)
        : LowHealthTrigger(botAI, "critical health", sPlayerbotAIConfig.criticalHealth, 0)
    {
    }
};

class MediumHealthTrigger : public LowHealthTrigger
{
public:
    MediumHealthTrigger(PlayerbotAI* botAI)
        : LowHealthTrigger(botAI, "medium health", sPlayerbotAIConfig.mediumHealth, 0)
    {
    }
};

class AlmostFullHealthTrigger : public LowHealthTrigger
{
public:
    AlmostFullHealthTrigger(PlayerbotAI* botAI)
        : LowHealthTrigger(botAI, "almost full health", sPlayerbotAIConfig.almostFullHealth,
                           sPlayerbotAIConfig.mediumHealth)
    {
    }
};

class PartyMemberLowHealthTrigger : public HealthInRangeTrigger
{
public:
    PartyMemberLowHealthTrigger(PlayerbotAI* botAI, std::string const name = "party member low health",
                                float value = sPlayerbotAIConfig.lowHealth,
                                float minValue = 0)
        : HealthInRangeTrigger(botAI, name, value, minValue)
    {
    }

    std::string const GetTargetName() override { return "party member to heal"; }
};

class PartyMemberCriticalHealthTrigger : public PartyMemberLowHealthTrigger
{
public:
    PartyMemberCriticalHealthTrigger(PlayerbotAI* botAI)
        : PartyMemberLowHealthTrigger(botAI, "party member critical health", sPlayerbotAIConfig.criticalHealth, 0)
    {
    }
};

class PartyMemberMediumHealthTrigger : public PartyMemberLowHealthTrigger
{
public:
    PartyMemberMediumHealthTrigger(PlayerbotAI* botAI)
        : PartyMemberLowHealthTrigger(botAI, "party member medium health", sPlayerbotAIConfig.mediumHealth,
                                      0)
    {
    }
};

class PartyMemberAlmostFullHealthTrigger : public PartyMemberLowHealthTrigger
{
public:
    PartyMemberAlmostFullHealthTrigger(PlayerbotAI* botAI)
        : PartyMemberLowHealthTrigger(botAI, "party member almost full health", sPlayerbotAIConfig.almostFullHealth,
                                      0)
    {
    }
};

class TargetLowHealthTrigger : public HealthInRangeTrigger
{
public:
    TargetLowHealthTrigger(PlayerbotAI* botAI, float value, float minValue = 0)
        : HealthInRangeTrigger(botAI, "target low health", value, minValue)
    {
    }

    std::string const GetTargetName() override { return "current target"; }
};

class TargetCriticalHealthTrigger : public TargetLowHealthTrigger
{
public:
    TargetCriticalHealthTrigger(PlayerbotAI* botAI) : TargetLowHealthTrigger(botAI, 20) {}
};

class HealerLowManaTrigger : public Trigger
{
public:
    HealerLowManaTrigger(PlayerbotAI* botAI) : Trigger(botAI, "healer low mana") {}

    std::string const GetTargetName() override { return "healer low mana"; }
    bool IsActive() override;
};

class PartyMemberDeadTrigger : public Trigger
{
public:
    PartyMemberDeadTrigger(PlayerbotAI* botAI) : Trigger(botAI, "resurrect", 1 * 1000) {}

    std::string const GetTargetName() override { return "party member to resurrect"; }
    bool IsActive() override;
};

class CombatPartyMemberDeadTrigger : public Trigger
{
public:
    CombatPartyMemberDeadTrigger(PlayerbotAI* ai) : Trigger(ai, "combat party member to resurrect", 1) {}
    std::string const GetTargetName() override { return "party member to resurrect"; }
    bool IsActive() override;
};

class DeadTrigger : public Trigger
{
public:
    DeadTrigger(PlayerbotAI* botAI) : Trigger(botAI, "dead") {}

    std::string const GetTargetName() override { return "self target"; }
    bool IsActive() override;
};

class AoeHealTrigger : public Trigger
{
public:
    AoeHealTrigger(PlayerbotAI* botAI, std::string const name, std::string const type, int32 count)
        : Trigger(botAI, name), count(count), type(type)
    {
    }  // reorder args - whipowill
    bool IsActive() override;

protected:
    int32 count;
    std::string const type;
};

// 补位治疗：正常治疗已阵亡或没蓝时，由还有蓝的非治疗、非坦克职业顶上一发。
// 因格瓦尔实测（19 场）：团灭时治疗死在 77–104 秒而击杀耗时 118–122 秒，
// 「队里没有活着且有蓝的治疗」这个窗口占 9.9 秒/场、出现在 7/19 场——整段无人治疗。
// 判据与 AN 层那版一致，额外排除坦克（坦克跑去补治疗等于丢仇恨）。
class PartyNeedsOffhealTrigger : public Trigger
{
public:
    PartyNeedsOffhealTrigger(PlayerbotAI* ai) : Trigger(ai, "party needs offheal", 1) {}
    bool IsActive() override;
};

// 该 bot 补位治疗用哪个法术；不会治疗的职业返回空串。
std::string const OffhealSpellName(Player* bot);

class AoeInGroupTrigger : public Trigger
{
public:
    AoeInGroupTrigger(PlayerbotAI* ai, std::string name, std::string type)
        : Trigger(ai, name), type(type)
    {
    }
    bool IsActive() override;

protected:
    std::string type;
};

#endif
