/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ElementalShamanStrategy.h"
#include "Playerbots.h"

ElementalShamanStrategy::ElementalShamanStrategy(PlayerbotAI* botAI) : GenericShamanStrategy(botAI)
{
    // No custom ActionNodeFactory needed
}

// ===== Default Actions =====
std::vector<NextAction> ElementalShamanStrategy::getDefaultActions()
{
    return {
        NextAction("lava burst", 5.2f),
        NextAction("lightning bolt", 5.0f)
    };
}

// ===== Trigger Initialization ===
void ElementalShamanStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    GenericShamanStrategy::InitTriggers(triggers);

    // Totem Triggers
    triggers.push_back(
        new TriggerNode(
            "call of the elements",
            {
                NextAction("call of the elements", 60.0f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "low health",
            {
                // 2026-09-17：试过把相关性从 40 提到 52，**没用、已回退**。
                // 根因不是排不上队：`CastStoneclawTotemAction::isUseful()` 是
                // `return !bot->GetGroup();` —— **只有单人时才算有用**，组队永远 USELESS。
                // 而且这很可能是**上游有意为之**：岩石外壳图腾会嘲讽附近的怪，
                // 组队时会把怪从坦克身上拉走。所以这条不是缺陷，别再动它。
                NextAction("stoneclaw totem", 40.0f)
            }
        )
    );

    // Cooldown Trigger
    triggers.push_back(
        new TriggerNode(
            "elemental mastery",
            {
                NextAction("elemental mastery", 29.0f)
            }
        )
    );

    // Damage Triggers
    triggers.push_back(
        new TriggerNode(
            "earth shock execute",
            {
                NextAction("earth shock", 5.5f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "flame shock",
            {
                NextAction("flame shock", 5.3f)
            }
        )
    );

    // Mana Triggers
    triggers.push_back(
        new TriggerNode(
            "water shield",
            {
                NextAction("water shield", 19.5f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "high mana",
            {
                NextAction("thunderstorm", 19.0f)
            }
        )
    );

    // Range Triggers
    triggers.push_back(
        new TriggerNode(
            "enemy is close",
            {
                NextAction("thunderstorm", 19.0f)
            }
        )
    );
}
