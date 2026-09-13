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
// 命中判定是 **boss 正面的锥**：EffectImplicitTargetA = 24 (TARGET_UNIT_CONE_ENEMY_24)，Spell.cpp 对该类型默认 24°，
// 但 world 库 `spell_cone` 表把 53472 / 59433 覆盖成 **120°**（Spell.cpp:1245 `sc->cone_degrees`），HasInArc 收全角 => ±60°；
// EffectRadiusIndex 给出锥长 **15 码**。boss 在 DoCast 前先 SELF_ROOT + DisableRotate(3,300 毫秒)，读条期间方向锁死，
// 目标是 10 码内随机一人（boss_anubarak.cpp EVENT_POUND）。
// 2026-09-13 复盘 run457–462：7 人次被 21k–33k 一击秒杀的非坦克全部离坦克 4–18 码、在 ±60° 内、±12° 外——
// 旧常量 24° 让躲踏动作对他们一律判"不用躲"。
constexpr float kPoundConeRadius = 15.0f;
constexpr float kPoundTargetRadius = 10.0f;  // boss_anubarak.cpp EVENT_POUND: SelectTarget(Random, 0, 10.0f)
constexpr float kPoundConeArc = 120.0f * float(M_PI) / 180.0f;   // HasInArc 收全角（spell_cone 覆盖值）

// 远程保距：站在 15 码锥长之外就永远不会被践踏打到，站在 10 码外就不会成为践踏目标。
constexpr float kRangedKeepDistance = 14.0f;   // 低于它触发（与目标距离留 4 码滞回，免得 boss 一挪就反复触发）
constexpr float kRangedKeepTarget   = 18.0f;   // 退到这么远
constexpr float kPoundMeleeBehindDistance = 5.0f;

// 场地西沿：run457–462 里 6 人次活着掉出平台，掉落前最后落点全部在 x≈525–533（地面 z≈223.4，开怪点 x=543）。
// 站位采样在 x≈525.1–526.5 形成一条竖线——那是导航网格的尽头，脚下 z 是网格给的，被穿刺(53454 效果 1 = 击退)一推就落空。
constexpr float kArenaSafeMinX  = 533.0f;   // 西沿：低于它触发护栏
constexpr float kArenaGuardX    = 536.0f;   // 西沿：目标点不低于这条线（与触发线留 3 码滞回）
// 平台整体近似圆形（`raidtest los` 探针 2 码网格，2026-09-13）：x 526–586（y=252）、y 224–276（x=550），西南角被削。
// 圆只用来筛候选点，最终以 vmap 地面校验（ResolveGround）为准。
constexpr float kArenaCenterX   = 553.0f;
constexpr float kArenaCenterY   = 250.0f;
constexpr float kArenaSafeRadius  = 22.0f;  // 离中心超过它触发护栏 / 候选点不得超过它
constexpr float kArenaGuardRadius = 18.0f;  // 护栏把人拉回到这个半径
constexpr uint32 kAnubarakEntry = 29120;

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

// 活着站到西沿安全线以外（x < kArenaSafeMinX）或离平台中心超过 kArenaSafeRadius。
class AnubarakRimTrigger : public Trigger
{
public:
    AnubarakRimTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak rim") {}
    bool IsActive() override;
};

// 远程 DPS 与治疗、且已在战斗中、离浮出的 boss 不足 kRangedKeepDistance（含体型半径）。
// 曾经把治疗排除、又曾经不限战斗状态：run 466/1 开怪前挪走牧师导致它整场留在非战斗引擎；run 467/1 牧师站在 boss 10 码内
// 被第一记践踏 27k 秒杀。现在的组合：战斗中 + 走位型（POSITIONING，施法可打断）。
class AnubarakRangedTooCloseTrigger : public Trigger
{
public:
    AnubarakRangedTooCloseTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak ranged too close") {}
    bool IsActive() override;
};

#endif
