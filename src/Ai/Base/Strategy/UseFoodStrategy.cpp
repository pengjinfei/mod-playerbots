/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UseFoodStrategy.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

void UseFoodStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    Strategy::InitTriggers(triggers);
    if (botAI->HasCheat(BotCheatMask::food))
    {
        triggers.push_back(new TriggerNode("medium health", { NextAction("food", 3.0f) }));
        triggers.push_back(new TriggerNode("high mana", { NextAction("drink", 3.0f) }));
    }
    else
    {
        // 吃东西同理：原来是 "low health"（< AiPlayerbot.LowHealth，默认 45），
        // 而「可以出战」的常规判据是 medium health（默认 65），于是 45%–65% 这一段
        // 背着食物也不吃、只能等自然回血。实测（mod-raidtest run380，英雄魔枢奥莫洛克）：
        // 清怪结束后全队法力够、但有人血量在 45%–65% 之间，恢复白等 11.8 / 12.2 秒且零进食。
        triggers.push_back(new TriggerNode("medium health", { NextAction("food", 3.0f) }));
        // 喝水的阈值与 cheat 分支一致（"high mana" = mana% < AiPlayerbot.HighMana，默认 65）。
        // 原来非 cheat 分支用的是 "low mana"（< AiPlayerbot.LowMana，默认 15），
        // 于是 15%–65% 这一段里 bot 明明背着水也不会喝，只能按脱战基础回复干等。
        // 实测（mod-raidtest run378，英雄魔枢）：戒律牧从 22.4% 起，105 秒的恢复窗口里
        // 一次没喝，法力曲线是 1,181/15 秒 = 78.7/s 的完美直线；同场法师在 15% 以下喝了一次，
        // 15 秒内从 22.7% 冲过 65%（约 392/s）——喝水本身是好的，缺的只是「该喝的时机」。
        // 开不开 food cheat 的差别本该只是水要不要白给，而不是什么时候该喝。
        triggers.push_back(new TriggerNode("high mana", { NextAction("drink", 3.0f) }));
    }
}
