#include "2s2h/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/BenPort.h"
#include "2s2h/Network/Anchor/CoopSync.h"

/**
 * UNSET_FLAG
 *
 * Fired when a flag is unset in the save context
 */

void Anchor::SendPacket_UnsetFlag(s16 sceneId, s16 flagType, s32 flag) {
    if (!IsSaveLoaded() || !roomState.syncItemsAndFlags) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = UNSET_FLAG;
    payload["targetTeamId"] = CVarGetString("gNetwork.Anchor.TeamId", "default");
    payload["addToQueue"] = true;
    payload["sceneId"] = sceneId;
    payload["flagType"] = flagType;
    payload["flag"] = flag;

    QueueOutgoingPacket(payload);
}

void Anchor::HandlePacket_UnsetFlag(nlohmann::json payload) {
    if (!IsSaveLoaded() || !roomState.syncItemsAndFlags) {
        return;
    }

    // MM Coop: desfaz a flag recebida (campanha vanilla). Valores fora da faixa sao descartados em CoopSync.
    s16 sceneId = payload.value("sceneId", (s16)SCENE_MAX);
    s16 flagType = payload.value("flagType", (s16)FLAG_NONE);
    s32 flag = payload.value("flag", (s32)-1);

    if (!CoopSync_ApplyFlag(sceneId, (FlagType)flagType, flag, false)) {
        SPDLOG_DEBUG("[Anchor] UNSET_FLAG ignorado: cena {} tipo {} flag {}", sceneId, flagType, flag);
    }
}
