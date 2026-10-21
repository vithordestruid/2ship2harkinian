#include "2s2h/Network/Anchor/Anchor.h"
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "2s2h/BenGui/Notification.h"
#include "2s2h/BenPort.h"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "z64cutscene.h"
#include "z64message.h"
extern PlayState* gPlayState;
}

/**
 * COOP_TIME (MM Coop, experimental)
 *
 * O dono da sala envia o dia e a hora do jogo a cada ~2 segundos. Quem ligou a opcao
 * "Sincronizar relogio" acompanha a hora do dono quando os dois estao no MESMO dia e o jogador
 * local nao esta em cutscene, dialogo ou pausa.
 *
 * Fica fora deste passo: dias diferentes e a Cancao do Tempo (ver plano do Marco F).
 */

namespace {
// Diferenca minima (cerca de 10 minutos do jogo) para corrigir. Evita "puxoes" a cada pacote.
constexpr s32 kTimeTolerance = CLOCK_TIME(0, 10);
s32 sLastWarnedOwnerDay = -1;

bool IsClockSyncEnabled() {
    return CVarGetInteger("gNetwork.Anchor.Coop.SyncClock", 0) != 0;
}

bool IsSafeToChangeTime(PlayState* play) {
    return play->pauseCtx.state == PAUSE_STATE_OFF && Message_GetState(&play->msgCtx) == TEXT_STATE_NONE &&
           CutsceneManager_GetCurrentCsId() == CS_ID_NONE && play->csCtx.state == CS_STATE_IDLE &&
           play->transitionTrigger == TRANS_TRIGGER_OFF;
}

// Distancia circular entre dois horarios (o relogio de 16 bits da volta a meia-noite).
s32 TimeDistance(u16 a, u16 b) {
    s32 diff = std::abs((s32)a - (s32)b);
    return diff > 0x8000 ? 0x10000 - diff : diff;
}
} // namespace

void Anchor::SendPacket_CoopTime() {
    if (!IsSaveLoaded() || !IsClockSyncEnabled() || roomState.ownerClientId != ownClientId) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = COOP_TIME;
    payload["targetTeamId"] = CVarGetString("gNetwork.Anchor.TeamId", "default");
    payload["day"] = gSaveContext.save.day;
    payload["time"] = gSaveContext.save.time;
    payload["quiet"] = true;

    QueueOutgoingPacket(payload);
}

void Anchor::HandlePacket_CoopTime(nlohmann::json payload) {
    if (!IsSaveLoaded() || !IsClockSyncEnabled() || roomState.ownerClientId == ownClientId) {
        return;
    }

    s32 ownerDay = payload.value("day", -1);
    s32 ownerTime = payload.value("time", -1);
    if (ownerDay < 0 || ownerDay > 4 || ownerTime < 0 || ownerTime > 0xFFFF) {
        return;
    }

    if (ownerDay != gSaveContext.save.day) {
        // Dias diferentes: nao mexe (pularia eventos da historia). Avisa uma vez por dia do dono.
        if (sLastWarnedOwnerDay != ownerDay) {
            sLastWarnedOwnerDay = ownerDay;
            Notification::Emit({
                .message = "Relogio: voces estao em dias diferentes. Dia do dono da sala:",
                .suffix = std::to_string(ownerDay),
            });
        }
        return;
    }
    sLastWarnedOwnerDay = -1;

    if (!IsSafeToChangeTime(gPlayState) || TimeDistance((u16)ownerTime, gSaveContext.save.time) < kTimeTolerance) {
        return;
    }

    SPDLOG_INFO("[Anchor] COOP_TIME: hora ajustada de {:#06x} para {:#06x} (dia {})", gSaveContext.save.time,
                ownerTime, ownerDay);
    gSaveContext.save.time = (u16)ownerTime;
}
