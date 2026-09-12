/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ANTRIGGERS_H
#define PLAYERBOTS_ANTRIGGERS_H

#include "DungeonStrategyUtils.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Trigger.h"

enum AzjolNerubIDs
{
    // Krik'thir the Gatewatcher
    NPC_KRIKTHIR                    = 28684,
    NPC_WATCHER_SILTHIK             = 28731,
    NPC_WATCHER_GASHRA              = 28730,
    NPC_WATCHER_NARJIL              = 28729,
    NPC_WATCHER_SKIRMISHER          = 28734,
    NPC_WATCHER_SHADOWCASTER        = 28733,
    NPC_WATCHER_WARRIOR             = 28732,
    DEBUFF_WEB_WRAP                 = 52086,
    NPC_WEB_WRAP                    = 28619,

    // Anub'arak
    SPELL_IMPALE_PERIODIC           = 53456,
    SPELL_IMPALE_SPIKES             = 53457,
    // 穿刺尖刺本体。上游注释原写「不是 gameobject 也不是触发器、瞬发没读条，追踪不到」——
    // 不成立：它就是一只 creature，FindNearestCreature 直接能找到，而且有 4 秒预警窗口。
    // SmartAI(29184)：t=0 给自己挂预警光环 53455 -> t=4000 施放 53454（英雄 59446）
    // -> t=4200 移除预警 -> 7 秒后消失。伤害半径只有 4.0 码（SpellRadius idx 26），
    // 英雄档 7,539 点 + 击退。run445 a3 实测它一场就打出 105,838（占全队承伤约四分之一）。
    NPC_IMPALE_TARGET               = 29184,
    SPELL_POUND_N                   = 53472,
    SPELL_POUND_H                   = 59433,
};

#define SPELL_POUND                 DUNGEON_MODE(bot, SPELL_POUND_N, SPELL_POUND_H)

class KrikthirWebWrapTrigger : public Trigger
{
public:
    KrikthirWebWrapTrigger(PlayerbotAI* ai) : Trigger(ai, "krik'thir web wrap") {}
    bool IsActive() override;
};

class KrikthirWatchersTrigger : public Trigger
{
public:
    KrikthirWatchersTrigger(PlayerbotAI* ai) : Trigger(ai, "krik'thir watchers") {}
    bool IsActive() override;
};

// 践踏（英雄 59433 读条 3,200 毫秒 -> 对每个命中者补 59432 伤害，满额不分摊，BasePoints 47,124）。
// 命中判定不是圆，是 **boss 正面的窄锥**：59433 的 EffectImplicitTargetA = 24
// (TARGET_UNIT_CONE_ENEMY_24)，Spell.cpp:1251 给这个目标类型写死 cone_degrees = 24，
// HasInArc 取半角 => **±12°**；EffectRadiusIndex 给出锥长 **15 码**。
// boss 在 DoCast 前先 SELF_ROOT + DisableRotate(3,300 毫秒)，所以读条期间锥子方向锁死不转。
// 实测布甲挨过 27,502 / 32,603 / 35,953 / 37,154 —— 约 2.5 倍血量上限，必死；坦克 17,153 能扛。
constexpr float kPoundConeRadius = 15.0f;
constexpr float kPoundConeArc = 24.0f * float(M_PI) / 180.0f;   // HasInArc 收全角

// 伤害半径 4.0 码；触发半径留 2 码余量，让 bot 有时间挪出去；挪到 8 码再留一档。
constexpr float kImpaleDamageRadius  = 4.0f;
constexpr float kImpaleTriggerRadius = 6.0f;
constexpr float kImpaleSafeDistance  = 8.0f;

// 身边最近的穿刺尖刺；没有则 nullptr。触发器与闪避动作共用同一判据。
Unit* FindNearestImpaleSpike(Player* bot, float range);

class AnubarakImpaleTrigger : public Trigger
{
public:
    AnubarakImpaleTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak impale") {}
    bool IsActive() override;
};

class AnubarakPoundTrigger : public Trigger
{
public:
    AnubarakPoundTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak pound") {}
    bool IsActive() override;
};

#endif
