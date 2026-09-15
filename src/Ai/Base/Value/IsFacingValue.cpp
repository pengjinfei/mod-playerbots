/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "IsFacingValue.h"
#include "Playerbots.h"
#include <cmath>

bool IsFacingValue::Calculate()
{
    Unit* target = AI_VALUE(Unit*, qualifier);
    if (!target)
        return false;

    // 核心自身判「面向」用的是 ±90°：Unit.cpp 里招架/格挡/命中方向、以及施法的 isInFront
    // 都是 HasInArc(M_PI, ...)（HasInArc 收全角）。这里原本写 M_PI_2 = ±45°，比游戏严一倍——
    // 中间那 45° 灰带里 bot 明明能正常攻击和施法，却每次目标微动就点亮 "not facing target"，
    // 推出 set facing（相关性 37，压在主输出技能之上；执行后还会 SetNextCheckDelay(ReactDelay=100ms)）。
    // 阿努巴拉克实测一场：盗贼 set facing 推入 978 次、执行 164 次（额外 16 秒停顿），
    // 而主输出毁伤 967 次推入只执行 114 次（12%）。
    // 留 5° 余量，避免正好卡在边界上施法被 SPELL_FAILED_UNIT_NOT_INFRONT 打回。
    static constexpr float kFacingArc = 170.0f * float(M_PI) / 180.0f;  // 全角 170° => ±85°
    return bot->HasInArc(kFacingArc, target);
}
