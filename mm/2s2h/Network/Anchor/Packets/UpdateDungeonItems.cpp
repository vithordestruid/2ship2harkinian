#include "2s2h/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/BenPort.h"
#include "2s2h/Network/Anchor/CoopSync.h"

/**
 * UPDATE_DUNGEON_ITEMS
 *
 * This is for 2 things, first is updating the dungeon items in vanilla saves, and second is
 * for ensuring the amount of keys used is synced as players are using them.
 */

void Anchor::SendPacket_UpdateDungeonItems() {
    if (!IsSaveLoaded() || !roomState.syncItemsAndFlags) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = UPDATE_DUNGEON_ITEMS;
    payload["targetTeamId"] = CVarGetString("gNetwork.Anchor.TeamId", "default");
    payload["addToQueue"] = true;
    // So dentro de masmorra; dungeonKeys tem 9 posicoes (menor que dungeonItems)
    if (!CoopSync_IsInDungeon()) {
        return;
    }
    payload["mapIndex"] = gSaveContext.mapIndex;
    payload["cycle"] = CoopSync_CurrentCycle();
    payload["dungeonItems"] = gSaveContext.save.saveInfo.inventory.dungeonItems[gSaveContext.mapIndex];
    payload["dungeonKeys"] = gSaveContext.save.saveInfo.inventory.dungeonKeys[gSaveContext.mapIndex];
    payload["strayFairies"] = gSaveContext.save.saveInfo.inventory.strayFairies[gSaveContext.mapIndex];

    QueueOutgoingPacket(payload);
}

void Anchor::HandlePacket_UpdateDungeonItems(nlohmann::json payload) {
    if (!IsSaveLoaded() || !roomState.syncItemsAndFlags) {
        return;
    }

    // MM Coop: chaves e fadas zeram a cada ciclo; ignora estado vindo de outro ciclo
    if (CoopSync_IsFromOtherCycle(payload.contains("cycle"), payload.value("cycle", (s32)-1))) {
        SPDLOG_DEBUG("[Anchor] UPDATE_DUNGEON_ITEMS ignorado: veio de outro ciclo de 3 dias");
        return;
    }

    // MM Coop: valida o indice antes de escrever no save (pacote vindo da rede)
    s32 mapIndex = payload.value("mapIndex", -1);
    if (mapIndex < 0 || mapIndex >= ARRAY_COUNT(gSaveContext.save.saveInfo.inventory.dungeonItems) ||
        mapIndex >= ARRAY_COUNT(gSaveContext.save.saveInfo.inventory.dungeonKeys) ||
        mapIndex >= ARRAY_COUNT(gSaveContext.save.saveInfo.inventory.strayFairies)) {
        SPDLOG_DEBUG("[Anchor] UPDATE_DUNGEON_ITEMS ignorado: mapIndex {}", mapIndex);
        return;
    }
    gSaveContext.save.saveInfo.inventory.dungeonItems[mapIndex] = payload.value("dungeonItems", (u8)0);
    gSaveContext.save.saveInfo.inventory.dungeonKeys[mapIndex] = payload.value("dungeonKeys", (s8)0);
    gSaveContext.save.saveInfo.inventory.strayFairies[mapIndex] = payload.value("strayFairies", (s8)0);
}
