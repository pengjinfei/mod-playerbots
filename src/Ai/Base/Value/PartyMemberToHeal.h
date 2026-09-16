/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_PARTYMEMBERTOHEAL_H
#define PLAYERBOTS_PARTYMEMBERTOHEAL_H

#include "PartyMemberValue.h"

class Pet;
class PlayerbotAI;
class Unit;

class PartyMemberToHeal : public PartyMemberValue
{
public:
    PartyMemberToHeal(PlayerbotAI* botAI, std::string const name = "party member to heal")
        : PartyMemberValue(botAI, name)
    {
    }

protected:
    Unit* Calculate() override;
    bool Check(Unit* player) override;
};

// 与 "party member to heal" 唯一的区别：**不按视线过滤候选**。
// 只给「走过去」这条链用（接近动作 + 它的触发器）：看不见的队友不等于不存在，
// 否则治疗会站在柱子后面对着满血的自己平A，而 18 码外的坦克被磨死
// （2026-09-15 因格瓦尔、2026-09-16 德雷德各复现一次；后者量到有视线 0.44 次治疗/秒、
//  无视线 0.13 次/秒）。**施法类动作仍用带视线的那个取值**，免得隔墙施法。
class PartyMemberToHealNoLos : public PartyMemberToHeal
{
public:
    PartyMemberToHealNoLos(PlayerbotAI* botAI, std::string const name = "party member to heal no los")
        : PartyMemberToHeal(botAI, name)
    {
    }

protected:
    bool Check(Unit* player) override;
};

class PartyMemberToProtect : public PartyMemberValue
{
public:
    PartyMemberToProtect(PlayerbotAI* botAI, std::string const name = "party member to protect")
        : PartyMemberValue(botAI, name)
    {
    }

protected:
    Unit* Calculate() override;
};

class HealerLowMana : public PartyMemberValue
{
public:
    HealerLowMana(PlayerbotAI* botAI) : PartyMemberValue(botAI, "healer low mana") {}

protected:
    Unit* Calculate() override;
};

#endif
