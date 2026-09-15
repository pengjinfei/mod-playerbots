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
// 治疗保距（第十三轮）：牧师躲踏只在 10 码内起作用，run 478/479 八场团灭里五场首死是站在 12.9–16.3 码被 28–35k 践踏秒杀
// （命中半径按 core 是 15 + 目标体型，16.3 码仍在内）。治疗阈值放到 17，退到 20，留出体型和 boss 挪动的余量。
constexpr float kHealerKeepDistance = 17.0f;
constexpr float kHealerKeepTarget   = 20.0f;
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

// 坦克在 boss 读条践踏时开圣佑术（498，−50%、2 分钟冷却）。
// run 457–473：坦克 109 次正常践踏均值 16k；三次 22k–31.6k 全部对应破甲 2–5 层，不是暴击。
// 第二十三轮试过收紧（破甲 >=5 且读条只剩 1.5 秒再开），run 515 直接 0/5、坦克五场全死：
// 触发器从 4.7 次/场掉到 2 次、圣佑从 1.3 掉到 0.6、>=8k 践踏的覆盖率 4%→0%。已回退。
// 教训：践踏读条 3.2 秒，但 tick 粒度加排队让「剩 1.5 秒再开」经常错过；
// 「白给一次圣佑」远好过「该开时没开」。破甲阈值也保持 3——守卫要到第一次潜地（约 50 秒）才刷，
// 抬到 5 只会让中段也开不出来。唯一放宽的是：boss <=25% 时即使破甲不足也允许开。
constexpr uint32 kPoundGuardSunderStacks = 3;
constexpr float kPoundGuardBossHealthPct = 25.0f;
class AnubarakPoundTankTrigger : public Trigger
{
public:
    AnubarakPoundTankTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak pound tank") {}
    bool IsActive() override;
};

// 治疗在 boss 读条践踏（3.2 秒）时预先给主坦上盾并接一记治疗。
class AnubarakPoundHealerTrigger : public Trigger
{
public:
    AnubarakPoundHealerTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak pound healer") {}
    bool IsActive() override;
};

// 毒疗者优先（第十六轮）：run 483–485 毒疗者每轮对全队 545–700k、承伤第一（毒箭齐射 59359 每发全队约 13k），每场 4 只、平均活 24 秒放 2 发；
// bot 打在它身上的伤害只占 14%。DPS（非坦克非治疗）在场上有活着的、已进入主坦 kVenomancerFocusAnchorRange 码内的毒疗者时，
// 把最近的一只写进 "prioritized targets"（DpsTargetValue 的 IsHighPriority 会先选它）；没有时清掉自己写的那条。
// 用主坦距离做门限是为了不让 DPS 盯着还在坡道上、追敌上限（38.5 码）够不到的目标发呆。
constexpr uint32 kVenomancerEntry = 29217;
constexpr float kVenomancerSearchRange = 60.0f;
constexpr float kVenomancerFocusAnchorRange = 40.0f;
Unit* FindVenomancerToFocus(PlayerbotAI* botAI, Player* bot);
class AnubarakVenomancerFocusTrigger : public Trigger
{
public:
    AnubarakVenomancerFocusTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak venomancer focus") {}
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
// 被第一记践踏 27k 秒杀。现在的组合：战斗中 + TACTICAL（施法不打断它）；远程 DPS 阈值 14→18，治疗阈值 17→20。
class AnubarakRangedTooCloseTrigger : public Trigger
{
public:
    AnubarakRangedTooCloseTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak ranged too close") {}
    bool IsActive() override;
};

// 近战 DPS 站进了 boss 的正面践踏锥（±60°、15 码）——该绕到背后去。
class AnubarakMeleeFrontTrigger : public Trigger
{
public:
    AnubarakMeleeFrontTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak melee front", 1) {}
    bool IsActive() override;
};

// 英勇/嗜血只该落在最后冲刺 = boss 第三次出土之后。
// 注意不能用「可选中且血 <=25%」来代替：血线判定与真正潜地之间有几十秒（run 512/1 实测 boss 191 秒
// 已经掉到 23% 且可选中，第三次潜地 217 秒才开始），那段窗口里放英勇照样会被潜地吃掉。
// 只能数出土次数：boss 潜地期带 UNIT_FLAG_NOT_SELECTABLE，从「不可选中」翻回「可选中」即一次出土。
bool AnubarakAfterThirdEmerge(PlayerbotAI* botAI, Player* bot);

// 依据（run 505-509 共 23 场）：判别输赢的是 180 秒时 boss 的血（击杀场中位 24%、失败场 41%），
// 而英勇在击杀场中位落在 213 秒、失败场落在 143 秒（第二次潜地附近，boss 根本打不到）。
// 共享层 HeroismTrigger 继承 BoostTrigger（balance<=50），不看阶段，打得顺就不放、打得乱就早放。
class AnubarakHeroismTrigger : public Trigger
{
public:
    AnubarakHeroismTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak heroism", 1) {}
    bool IsActive() override;
};

// 治疗已经接不住（没蓝或阵亡）且有人残血时，让非治疗职业补一发治疗波。
// 依据（run 483-493 + 500/501 共 60 场）：坦克阵亡 39 场里 31 场死前 3 秒 boss 一下没打到——
// 是守卫(53.5%)+毒疗者(35.7%)磨死的，同时刻牧师蓝中位 38 点；而元素萨满 18,475 的法力池整场只放 0.6 次治疗波。
class AnubarakOffhealTrigger : public Trigger
{
public:
    AnubarakOffhealTrigger(PlayerbotAI* ai) : Trigger(ai, "anub'arak offheal", 1) {}
    bool IsActive() override;
    std::string const GetTargetName() override { return "party member to heal"; }
};

#endif
