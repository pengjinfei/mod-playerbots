/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "PartyMemberToHeal.h"
#include "Playerbots.h"
#include "ServerFacade.h"

class IsTargetOfHealingSpell : public SpellEntryPredicate
{
public:
    bool Check(SpellInfo const* spellInfo) override
    {
        for (uint8 i = 0; i < 3; ++i)
        {
            if (spellInfo->Effects[i].Effect == SPELL_EFFECT_HEAL ||
                spellInfo->Effects[i].Effect == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                spellInfo->Effects[i].Effect == SPELL_EFFECT_HEAL_MECHANICAL)
                return true;
        }

        return false;
    }
};

inline bool compareByHealth(Unit const* u1, Unit const* u2) { return u1->GetHealthPct() < u2->GetHealthPct(); }

namespace
{
// 治疗把自己排在最后是会死的。因格瓦尔 2026-09-15 十场实测：治疗在 50% 血以下停留 **94 秒**，
// 其中只有 **12 秒**收到过任何治疗（含 HoT 跳动）；七次阵亡里，死前 12 秒收到的治疗是 0–4 次。
// 不是没蓝（死时法力 11k/16.7k），是取值层的排序：probeValue = 血量% + 到自己的距离/10，
// 自己的距离项恒为 0，而坦克隔着 25 码也只加 2.5——坦克 46+2.5=48.5 长期以微弱优势压过自己 49+0=49，
// 于是治疗永远是"第二低"，永远轮不到自己。而它唯一的自保动作是 criticalHealth(25%) 的痛苦压制，
// 一记 Dreadful Roar 就是血池的 25–40%，从 49% 直接到死，那条线根本来不及。
//
// 真人治疗的做法是：自己掉到中等血量就先把自己垫起来，除非队友已经危险。这里按同一条规则收口：
// 自己是治疗、血量低于 mediumHealth，且当前选中的目标**并不危险**（高于 lowHealth）时，先治自己。
// 队友一旦低于 lowHealth 就重新压过自己，不会出现"治疗光顾自己让坦克死"的反向问题。
Unit* PreferSelfWhenHealerIsWounded(PlayerbotAI* botAI, Player* bot, Unit* target)
{
    if (!botAI->IsHeal(bot) || !bot->IsAlive())
        return target;

    if (bot->GetHealthPct() >= sPlayerbotAIConfig.mediumHealth)
        return target;

    if (target && target != bot && target->GetHealthPct() <= sPlayerbotAIConfig.lowHealth)
        return target;

    return bot;
}
}  // namespace

Unit* PartyMemberToHeal::Calculate()
{
    IsTargetOfHealingSpell predicate;

    Group* group = bot->GetGroup();
    if (!group)
        return bot;

    bool isRaid = bot->GetGroup()->isRaidGroup();
    MinValueCalculator calc(100);

    // If focus heal targets strategy is active, only heal those targets
    if (botAI->HasStrategy("focus heal targets", BOT_STATE_COMBAT))
    {
        std::list<ObjectGuid> const focusHealTargets =
            AI_VALUE(std::list<ObjectGuid>, "focus heal targets");

        for (ObjectGuid const& focusHealTarget : focusHealTargets)
        {
            Player* player = ObjectAccessor::FindPlayer(focusHealTarget);
            if (!player || !player->IsInWorld() || !player->IsAlive() || !player->IsInSameGroupWith(bot))
                continue;

            float health = player->GetHealthPct();
            if (isRaid || health < sPlayerbotAIConfig.mediumHealth ||
                !IsTargetOfSpellCast(player, predicate))
            {
                float probeValue = 100.0f;
                if (player->GetDistance2d(bot) > sPlayerbotAIConfig.healDistance)
                    probeValue = health + 30.0f;
                else
                    probeValue = health + player->GetDistance2d(bot) / 10.0f;

                if (probeValue < calc.minValue && Check(player))
                    calc.probe(probeValue, player);
            }
        }

        return PreferSelfWhenHealerIsWounded(botAI, bot, (Unit*)calc.param);
    }

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* player = gref->GetSource();
        if (player->IsGameMaster())
            continue;
        if (player && player->IsAlive())
        {
            float health = player->GetHealthPct();
            if (isRaid || health < sPlayerbotAIConfig.mediumHealth || !IsTargetOfSpellCast(player, predicate))
            {
                float probeValue = 100.0f;
                if (player->GetDistance2d(bot) > sPlayerbotAIConfig.healDistance)
                {
                    probeValue = health + 30.0f;
                }
                else
                {
                    probeValue = health + player->GetDistance2d(bot) / 10.0f;
                }
                // delay Check player to here for better performance
                if (probeValue < calc.minValue && Check(player))
                {
                    calc.probe(probeValue, player);
                }
            }
        }

        Pet* pet = player->GetPet();
        if (pet && pet->IsAlive())
        {
            float health = ((Unit*)pet)->GetHealthPct();
            float probeValue = 100.0f;
            if (isRaid || health < sPlayerbotAIConfig.mediumHealth)
                probeValue = health + 30.0f;
            // delay Check pet to here for better performance
            if (probeValue < calc.minValue && Check(pet))
            {
                calc.probe(probeValue, pet);
            }
        }

        Unit* charm = player->GetCharm();
        if (charm && charm->IsAlive())
        {
            float health = charm->GetHealthPct();
            float probeValue = 100.0f;
            if (isRaid || health < sPlayerbotAIConfig.mediumHealth)
                probeValue = health + 30.0f;
            // delay Check charm to here for better performance
            if (probeValue < calc.minValue && Check(charm))
            {
                calc.probe(probeValue, charm);
            }
        }
    }
    return PreferSelfWhenHealerIsWounded(botAI, bot, (Unit*)calc.param);
}

bool PartyMemberToHeal::Check(Unit* player)
{
    // return player && player != bot && player->GetMapId() == bot->GetMapId() && player->IsInWorld() &&
    //     ServerFacade::instance().GetDistance2d(bot, player) < (player->IsPlayer() && botAI->IsTank((Player*)player) ? 50.0f
    //     : 40.0f);
    return player->GetMapId() == bot->GetMapId() && !player->IsCharmed() &&
           bot->GetDistance2d(player) < sPlayerbotAIConfig.healDistance * 2 && bot->IsWithinLOSInMap(player);
}

bool PartyMemberToHealNoLos::Check(Unit* player)
{
    // 与基类同一套判据，去掉 IsWithinLOSInMap。距离仍然限制在治疗距离的两倍内。
    return player->GetMapId() == bot->GetMapId() && !player->IsCharmed() &&
           bot->GetDistance2d(player) < sPlayerbotAIConfig.healDistance * 2;
}

Unit* HealerLowMana::Calculate()
{
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    MinValueCalculator calc(100);

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* player = gref->GetSource();
        if (!player || player == bot)
            continue;
        if (player->IsGameMaster() || !player->IsAlive())
            continue;
        if (!botAI->IsHeal(player))
            continue;

        float mana = player->GetPowerPct(POWER_MANA);
        if (mana < calc.minValue)
            calc.probe(mana, player);
    }

    return (Unit*)calc.param;
}

Unit* PartyMemberToProtect::Calculate()
{
    return nullptr;
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    std::vector<Unit*> needProtect;

    GuidVector attackers = botAI->GetAiObjectContext()->GetValue<GuidVector>("attackers")->Get();
    for (GuidVector::iterator i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit)
            continue;

        Unit* pVictim = unit->GetVictim();
        if (!pVictim || !pVictim->IsPlayer())
            continue;

        if (pVictim == bot)
            continue;

        float attackDistance = 30.0f;
        if (ServerFacade::instance().GetDistance2d(pVictim, unit) > attackDistance)
            continue;

        if (botAI->IsTank((Player*)pVictim) && pVictim->GetHealthPct() > 10)
            continue;
        else if (pVictim->GetHealthPct() > 30)
            continue;

        if (find(needProtect.begin(), needProtect.end(), pVictim) == needProtect.end())
            needProtect.push_back(pVictim);
    }

    if (needProtect.empty())
        return nullptr;

    sort(needProtect.begin(), needProtect.end(), compareByHealth);

    return needProtect[0];
}
