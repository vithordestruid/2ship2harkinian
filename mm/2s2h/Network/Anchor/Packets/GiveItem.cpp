#include "2s2h/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/BenGui/Notification.h"
#include "2s2h/BenPort.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/Network/Anchor/CoopSync.h"

extern "C" {
#include "functions.h"
extern PlayState* gPlayState;
extern s16 D_801CFF94[250];
extern TexturePtr gItemIcons[131];
}

/**
 * GIVE_ITEM
 */

void Anchor::SendPacket_GiveItem(u16 modId, s16 getItemId, std::string targetTeamId) {
    if (!IsSaveLoaded() || !roomState.syncItemsAndFlags) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = GIVE_ITEM;
    payload["targetTeamId"] = targetTeamId == "" ? CVarGetString("gNetwork.Anchor.TeamId", "default") : targetTeamId;
    payload["addToQueue"] = true;
    payload["modId"] = modId;
    payload["getItemId"] = getItemId;

    QueueOutgoingPacket(payload);
}

void Anchor::HandlePacket_GiveItem(nlohmann::json payload) {
    if (!IsSaveLoaded() || !roomState.syncItemsAndFlags) {
        return;
    }

    uint32_t clientId = payload["clientId"].get<uint32_t>();
    AnchorClient& client = clients[clientId];

    // MM Coop: item da campanha vanilla (sem Randomizer)
    if (payload.contains("vanillaItem")) {
        int itemValue = payload.value("vanillaItem", -1);
        if (itemValue < 0 || itemValue > 0xFF) {
            return;
        }
        u8 item = (u8)itemValue;
        if (CoopSync_ApplyItem(item)) {
            Notification::Emit({
                .itemIcon = item < ARRAY_COUNT(gItemIcons) ? (const char*)gItemIcons[item] : nullptr,
                .prefix = client.name,
                .message = "compartilhou um item",
            });
        } else {
            SPDLOG_DEBUG("[Anchor] GIVE_ITEM vanilla ignorado: item {}", item);
        }
        return;
    }

    RandoItemId randoItemId = Rando::ConvertItem((RandoItemId)payload["getItemId"].get<s16>());
    std::string suffix = Rando::StaticData::GetItemName(randoItemId, false);

    if (randoItemId == RI_JUNK) {
        randoItemId = Rando::CurrentJunkItem();
    }

    if (Rando::StaticData::Items[randoItemId].randoItemType != RITYPE_JUNK) {
        Notification::Emit({
            .itemIcon = Rando::StaticData::GetIconTexturePath(randoItemId),
            .prefix = client.name,
            .message = "found your",
            .suffix = suffix,
        });
    }

    Rando::GiveItem(randoItemId);
}

/**
 * MM Coop: GIVE_ITEM da campanha vanilla.
 * Usa o mesmo tipo de pacote do Anchor, com o campo "vanillaItem" (ItemId do jogo) no lugar do item do Randomizer.
 */
void Anchor::SendPacket_GiveVanillaItem(u8 item) {
    if (!IsSaveLoaded() || !roomState.syncItemsAndFlags) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = GIVE_ITEM;
    payload["targetTeamId"] = CVarGetString("gNetwork.Anchor.TeamId", "default");
    payload["addToQueue"] = true;
    payload["vanillaItem"] = item;

    QueueOutgoingPacket(payload);
}
