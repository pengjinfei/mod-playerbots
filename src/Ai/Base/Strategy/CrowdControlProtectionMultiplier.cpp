/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "CrowdControlProtectionMultiplier.h"
#include "GenericSpellActions.h"
#include "Playerbots.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

namespace
{
    // 落点判定留的余量：控住的怪与队伍都可能在挪动，边界上宁可保守一点。
    constexpr float kAoeMargin = 2.0f;
}

bool CrowdControlProtectionMultiplier::HasBreakableFriendlyCc(Unit* unit) const
{
    for (auto const& applied : unit->GetAppliedAuras())
    {
        AuraApplication const* application = applied.second;
        if (!application)
            continue;

        Aura* aura = application->GetBase();
        if (!aura)
            continue;

        SpellInfo const* auraInfo = aura->GetSpellInfo();
        if (!auraInfo || !(auraInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_VICTIM))
            continue;

        // 判据与核心的官方定义同源：Unit::HasBreakableByDamageCrowdControlAura()
        // （Unit.cpp:949-970）用的就是 AuraInterruptFlags & AURA_INTERRUPT_FLAG_TAKE_DAMAGE
        // 加上 {CONFUSE, FEAR, STUN, ROOT, TRANSFORM} 这组光环类型。
        // 这里**故意去掉 MOD_ROOT**：被定住的怪照样在打人，用 AoE 打破它是正常打法，
        // 护着它等于白亏输出。
        // 实测（mod-raidtest run385/attempt1）：最初只判「受伤即掉」这一个标志位时，
        // 冰霜新星(42917) 的 8 秒定身把全队 AoE 压死了整整 8.3 秒
        // （14.3 秒新星 -> 22.6 秒才出现下一个奉献），奥莫洛克那组随之从 6/8 掉到 1/4。
        bool incapacitates = false;
        for (uint8 effect = EFFECT_0; effect <= EFFECT_2 && !incapacitates; ++effect)
        {
            switch (auraInfo->Effects[effect].ApplyAuraName)
            {
                case SPELL_AURA_MOD_CONFUSE:          // 变形术、致盲
                case SPELL_AURA_MOD_FEAR:             // 恐惧类
                case SPELL_AURA_MOD_STUN:             // 闷棍、冰冻陷阱
                case SPELL_AURA_MOD_PACIFY_SILENCE:   // 妖术
                case SPELL_AURA_TRANSFORM:            // 变形术、妖术的变形部分
                    incapacitates = true;
                    break;
                default:
                    break;
            }
        }

        if (!incapacitates)
            continue;

        // 只保护友方施加的控制。怪物自己身上「受伤即掉」的光环（例如某些伪装/潜行）
        // 不是我们要护着的东西，压住 AoE 反而是白亏输出。
        Unit* caster = aura->GetCaster();
        if (caster && bot->IsFriendlyTo(caster))
            return true;
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
    if (!spellInfo || !spellInfo->IsTargetingArea() || spellInfo->IsPositive())
        return 1.0f;

    float radius = 0.0f;
    for (uint8 effect = EFFECT_0; effect <= EFFECT_2; ++effect)
        radius = std::max(radius, spellInfo->Effects[effect].CalcRadius(bot));

    if (radius <= 0.0f)
        return 1.0f;

    // 圆心：以自身为中心的（新星/奉献/刀扇）与以目标为中心的（暴风雪/烈焰风暴）两类都存在，
    // 而施法前无法确知地面法术的落点。两个圆心都查一遍，宁可多让路。
    Unit* const currentTarget = action->GetTarget();

    GuidVector attackers = AI_VALUE(GuidVector, "attackers");
    for (ObjectGuid const& guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit == currentTarget)
            continue;

        if (!HasBreakableFriendlyCc(unit))
            continue;

        if (bot->GetDistance(unit) <= radius + kAoeMargin)
            return 0.0f;

        if (currentTarget && currentTarget->GetDistance(unit) <= radius + kAoeMargin)
            return 0.0f;
    }

    return 1.0f;
}
