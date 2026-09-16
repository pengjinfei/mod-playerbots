/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_PARTYMEMBERVALUE_H
#define PLAYERBOTS_PARTYMEMBERVALUE_H

#include "Player.h"
#include "Value.h"

class PlayerbotAI;

class FindPlayerPredicate
{
public:
    virtual ~FindPlayerPredicate() = default;
    virtual bool Check(Unit* /*unit*/) = 0;
};

class SpellEntryPredicate
{
public:
    virtual bool Check(SpellInfo const* /*spellInfo*/) = 0;
};

class PartyMemberValue : public UnitCalculatedValue
{
public:
    PartyMemberValue(PlayerbotAI* botAI, std::string const name = "party member", int checkInterval = 1)
        : UnitCalculatedValue(botAI, name, checkInterval)
    {
    }

    bool IsTargetOfSpellCast(Player* target, SpellEntryPredicate& predicate);

protected:
    Unit* FindPartyMember(FindPlayerPredicate& predicate, bool ignoreOutOfGroup = false);
    Unit* FindPartyMember(std::vector<Player*>* party, FindPlayerPredicate& predicate);
    virtual bool Check(Unit* player);
};

class PartyMemberMainTankValue : public PartyMemberValue
{
public:
    PartyMemberMainTankValue(PlayerbotAI* botAI) : PartyMemberValue(botAI, "main tank member", 2 * 1000) {}
    virtual Unit* Calculate();

protected:
    // 「谁是主坦」是**事实**，与看不看得见无关。基类 Check 把 IsWithinLOS 当候选过滤条件，
    // 于是被柱子挡住的那一秒这个值变空，而所有 `if (!mainTank) return false;` 的行为
    // （盗贼嫁祸于人/消失、法师隐形、牧师渐隐、给坦克的 buff，以及共享层"追敌不离主坦"那道护栏）
    // 整组静默跳过——引擎日志里连 PUSH 行都没有。
    // 实测（英雄达克萨隆要塞 King Dred，run 570，已剔除坦克阵亡后的采样）：非坦克位
    // **20%** 的战斗秒取不到主坦（法师 30% / 萨满 27% / 牧师 15% / 盗贼 6%）。
    // 这里只去掉视线，保留同地图与距离判据。**施法型取值（驱散/复活/buff）不动**——
    // 那些确实需要视线，隔墙放不出去。
    bool Check(Unit* player) override;
};

#endif
