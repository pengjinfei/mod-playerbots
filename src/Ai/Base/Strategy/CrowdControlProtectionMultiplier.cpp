/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "CrowdControlProtectionMultiplier.h"
#include "GenericSpellActions.h"
#include "Group.h"
#include "Log.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <set>

namespace
{
    // 「附近」的口径：一次拉怪的范围。再远的被控怪不属于这场战斗。
    constexpr float kProtectRange = 45.0f;

    // 与 RtiTargetValue::GetRtiIndex 一致：月亮(4) / 方块(5) / 十字(6) 是控制图标，骷髅(7) 是击杀。
    constexpr uint8 kCcIcons[] = { 4, 5, 6 };
}

bool CrowdControlProtectionMultiplier::IsMultiTargetSpell(SpellInfo const* spellInfo, std::string const& actionSpell) const
{
    // IsTargetingArea：目标从区域里选（烈焰风暴、冰霜新星、刀扇、锥形）。
    // IsAffectingArea：还包括持续性地面 AoE（奉献、暴风雪、烈焰风暴的余烬）——run393/attempt2 里
    // 暴风雪(42940) 的效果是 PERSISTENT_AREA_AURA，IsTargetingArea 对它是 false，羊被自家暴风雪
    // 一跳打掉。
    if (spellInfo->IsTargetingArea() || spellInfo->IsAffectingArea())
        return true;

    // 链式跳转（闪电链、正义之锤、顺劈、复仇者之盾）同样会砸到旁边被控的怪。
    for (uint8 effect = EFFECT_0; effect <= EFFECT_2; ++effect)
        if (spellInfo->Effects[effect].ChainTarget > 1)
            return true;

    return IsIndirectAoe(actionSpell);
}

bool CrowdControlProtectionMultiplier::IsIndirectAoe(std::string const& actionSpell) const
{
    // 法术数据上看不出来的间接 AoE：施放本身是单体/自身增益，伤害在之后落到周围的怪身上。
    // 这份名单按 bot 实际会用的动作名列，而不是想穷举游戏里的所有法术。
    static char const* const kIndirectAoe[] =
    {
        "living bomb",          // 到期爆炸，10 码范围
        "blade flurry",         // 每次攻击额外打一个邻近目标
        "killing spree",        // 在周围敌人之间跳
        "magma totem",          // 持续脉冲
        "fire elemental totem",
        "sweeping strikes",
        "bladestorm",
        "starfall",
        "seed of corruption",
    };
    for (char const* name : kIndirectAoe)
        if (actionSpell == name)
            return true;

    return false;
}

bool CrowdControlProtectionMultiplier::IsCrowdControlIconTarget(Unit* unit) const
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (uint8 icon : kCcIcons)
        if (group->GetTargetIcon(icon) == unit->GetGUID())
            return true;

    return false;
}

bool CrowdControlProtectionMultiplier::HasFriendlyCrowdControl(Unit* unit) const
{
    for (auto const& applied : unit->GetAppliedAuras())
    {
        AuraApplication const* application = applied.second;
        Aura* aura = application ? application->GetBase() : nullptr;
        SpellInfo const* auraInfo = aura ? aura->GetSpellInfo() : nullptr;
        if (!auraInfo)
            continue;

        // 只保护友方施加的控制。怪物自己身上的变形/潜行不是我们要护着的东西。
        Unit* caster = aura->GetCaster();
        if (!caster || !bot->IsFriendlyTo(caster))
            continue;

        for (uint8 effect = EFFECT_0; effect <= EFFECT_2; ++effect)
        {
            switch (auraInfo->Effects[effect].ApplyAuraName)
            {
                case SPELL_AURA_MOD_CONFUSE:          // 变形术、致盲
                case SPELL_AURA_MOD_FEAR:             // 恐惧类
                case SPELL_AURA_MOD_PACIFY_SILENCE:   // 妖术
                case SPELL_AURA_TRANSFORM:            // 变形术、妖术的变形部分
                    return true;
                case SPELL_AURA_MOD_STUN:             // 只认闷棍；制裁之锤/肾击是输出手段
                    if (auraInfo->Mechanic == MECHANIC_SAPPED ||
                        auraInfo->Effects[effect].Mechanic == MECHANIC_SAPPED)
                        return true;
                    break;
                default:
                    break;
            }
        }
    }

    return false;
}

float CrowdControlProtectionMultiplier::GetValue(Action* action)
{
    CastSpellAction* cast = dynamic_cast<CastSpellAction*>(action);
    if (!cast)
        return 1.0f;

    uint32 const spellId = AI_VALUE2(uint32, "spell id", cast->getSpell());
    if (!spellId)
        return 1.0f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return 1.0f;

    // 先查名单再看正负：剑刃乱舞/杀戮盛宴/活体炸弹这类间接 AoE 的施放本身是**正面**自身增益，
    // 用 IsPositive() 先放行会把它们漏掉（run404–407 每场开怪 15 秒左右剑刃乱舞都放出来了）。
    if (!IsMultiTargetSpell(spellInfo, cast->getSpell()))
        return 1.0f;
    if (spellInfo->IsPositive() && !IsIndirectAoe(cast->getSpell()))
        return 1.0f;

    // 候选：仇恨表里的怪 + 三个控制图标所指（被控住、没进过仇恨表的怪不在 "attackers" 里）。
    std::set<ObjectGuid> candidates;
    for (ObjectGuid const& guid : AI_VALUE(GuidVector, "attackers"))
        candidates.insert(guid);

    if (Group* group = bot->GetGroup())
        for (uint8 icon : kCcIcons)
            if (ObjectGuid guid = group->GetTargetIcon(icon))
                candidates.insert(guid);

    Unit* const actionTarget = action->GetTarget();
    for (ObjectGuid const& guid : candidates)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->IsFriendlyTo(bot) || unit == actionTarget)
            continue;

        if (bot->GetDistance(unit) > kProtectRange)
            continue;

        if (IsCrowdControlIconTarget(unit) || HasFriendlyCrowdControl(unit))
        {
            // 与上游 CanCastSpell 的失败日志同一开关（AiPlayerbot.LogInGroupOnly = 0 时可见）。
            if (!sPlayerbotAIConfig.logInGroupOnly)
                LOG_DEBUG("playerbots", "cc protection: {} holds {} because {} is crowd controlled",
                          bot->GetName(), cast->getSpell(), unit->GetName());
            return 0.0f;
        }
    }

    return 1.0f;
}
