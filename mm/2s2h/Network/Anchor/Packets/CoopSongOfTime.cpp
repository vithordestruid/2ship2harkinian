#include "2s2h/Network/Anchor/Anchor.h"
#include <chrono>
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "2s2h/BenGui/Notification.h"
#include "2s2h/BenPort.h"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "z64cutscene.h"
#include "z64message.h"
#include "z64save.h"
#include "z64scene.h"
extern PlayState* gPlayState;
}

/**
 * COOP_SONG_OF_TIME (MM Coop, experimental)
 *
 * Regra combinada: quando um jogador toca a Cancao do Tempo e confirma, o outro recebe um aviso e
 * volta ao Dia 1 quando confirmar (botao no menu do Anchor) ou quando acabarem 30 segundos.
 * Assim os dois nunca ficam em ciclos diferentes.
 *
 * O reinicio remoto repete o que o jogo faz:
 *  1. salvamento de fim de ciclo, igual a MSGMODE_NEW_CYCLE_0 (z_message.c) e ao MoonCrashSave do 2S2H;
 *  2. espera a gravacao terminar (sramCtx.status == 0), igual a MSGMODE_NEW_CYCLE_1;
 *  3. transicao para a cutscene do amanhecer do Dia 1, igual ao Player com OCARINA_MODE_APPLY_SOT (z_player.c).
 *
 * Fica fora deste passo: Cancao do Tempo Invertida e Cancao de Tempo Dupla.
 */

namespace {
using Clock = std::chrono::steady_clock;

constexpr auto kAutoResetDelay = std::chrono::seconds(30);
// Ignora pedidos que chegam logo depois de um reinicio local (os dois tocaram ao mesmo tempo).
constexpr auto kIgnoreAfterLocalReset = std::chrono::seconds(60);

enum class ResetState {
    None,         // nada pendente
    Pending,      // aviso recebido, esperando confirmacao ou o prazo
    WaitingFlash, // save de fim de ciclo feito, esperando a gravacao terminar
};

ResetState sState = ResetState::None;
Clock::time_point sPendingSince;
Clock::time_point sLastLocalReset;
bool sHasLocalReset = false;
bool sApplyingRemoteReset = false;
std::string sRequesterName;

bool IsSongOfTimeSyncEnabled() {
    return CVarGetInteger("gNetwork.Anchor.Coop.SyncSongOfTime", 0) != 0;
}

bool IsSafeToReset(PlayState* play) {
    return play->pauseCtx.state == PAUSE_STATE_OFF && Message_GetState(&play->msgCtx) == TEXT_STATE_NONE &&
           CutsceneManager_GetCurrentCsId() == CS_ID_NONE && play->csCtx.state == CS_STATE_IDLE &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->msgCtx.ocarinaMode == OCARINA_MODE_NONE;
}

// Passo 1: igual a MSGMODE_NEW_CYCLE_0 (z_message.c)
void DoEndOfCycleSave(PlayState* play) {
    play->state.unk_A3 = 1;
    u16 cutsceneIndex = gSaveContext.save.cutsceneIndex;
    u16 time = CURRENT_TIME;
    s32 day = gSaveContext.save.day;

    sApplyingRemoteReset = true;
    Sram_SaveEndOfCycle(play);
    sApplyingRemoteReset = false;

    gSaveContext.timerStates[TIMER_ID_MOON_CRASH] = TIMER_STATE_OFF;
    func_8014546C(&play->sramCtx);

    gSaveContext.save.day = day;
    gSaveContext.save.time = time;
    gSaveContext.save.cutsceneIndex = cutsceneIndex;

    if (gSaveContext.fileNum != 0xFF) {
        Sram_SetFlashPagesDefault(&play->sramCtx,
                                  gFlashSaveStartPages[gSaveContext.fileNum * FLASH_SAVE_MAIN_MULTIPLIER],
                                  gFlashSpecialSaveNumPages[gSaveContext.fileNum * FLASH_SAVE_MAIN_MULTIPLIER]);
        Sram_StartWriteToFlashDefault(&play->sramCtx);
    }
}

// Passo 3: igual ao Player quando ocarinaMode == OCARINA_MODE_APPLY_SOT (z_player.c)
void StartDawnTransition(PlayState* play) {
    if (gSaveContext.save.saveInfo.playerData.threeDayResetCount == 1) {
        play->nextEntrance = ENTRANCE(CUTSCENE, 1);
    } else {
        play->nextEntrance = ENTRANCE(CUTSCENE, 0);
    }
    gSaveContext.nextCutsceneIndex = 0xFFF7;
    play->transitionTrigger = TRANS_TRIGGER_START;
}
} // namespace

bool CoopSongOfTime_IsApplyingRemoteReset() {
    return sApplyingRemoteReset;
}

bool CoopSongOfTime_IsPending() {
    return sState == ResetState::Pending;
}

void CoopSongOfTime_ConfirmNow() {
    if (sState == ResetState::Pending) {
        // Antecipa o prazo: o reinicio acontece no proximo quadro seguro
        sPendingSince = Clock::now() - kAutoResetDelay;
    }
}

void Anchor::SendPacket_CoopSongOfTime() {
    if (!IsSaveLoaded() || !IsSongOfTimeSyncEnabled()) {
        return;
    }
    sHasLocalReset = true;
    sLastLocalReset = Clock::now();

    nlohmann::json payload;
    payload["type"] = COOP_SONG_OF_TIME;
    payload["targetTeamId"] = CVarGetString("gNetwork.Anchor.TeamId", "default");
    QueueOutgoingPacket(payload);
}

void Anchor::HandlePacket_CoopSongOfTime(nlohmann::json payload) {
    if (!IsSaveLoaded() || !IsSongOfTimeSyncEnabled() || sState != ResetState::None) {
        return;
    }
    if (sHasLocalReset && Clock::now() - sLastLocalReset < kIgnoreAfterLocalReset) {
        SPDLOG_INFO("[Anchor] COOP_SONG_OF_TIME ignorado: reinicio local recente");
        return;
    }

    uint32_t clientId = payload.value("clientId", (uint32_t)0);
    sRequesterName = clients.contains(clientId) ? clients[clientId].name : "Parceiro";
    sState = ResetState::Pending;
    sPendingSince = Clock::now();

    SPDLOG_INFO("[Anchor] COOP_SONG_OF_TIME recebido de {}", sRequesterName);
    Notification::Emit({
        .prefix = sRequesterName,
        .message = "tocou a Cancao do Tempo. Voltando ao Dia 1 em 30 s (ou confirme no menu do Anchor)",
        .remainingTime = 10.0f,
    });
}

// Chamado a cada quadro (OnGameStateMainStart)
void Anchor::CoopSongOfTime_Tick() {
    if (sState == ResetState::None || !IsSaveLoaded()) {
        return;
    }
    PlayState* play = gPlayState;

    if (sState == ResetState::Pending) {
        if (Clock::now() - sPendingSince < kAutoResetDelay || !IsSafeToReset(play)) {
            return;
        }
        SPDLOG_INFO("[Anchor] COOP_SONG_OF_TIME: salvando fim de ciclo");
        DoEndOfCycleSave(play);
        sState = ResetState::WaitingFlash;
        return;
    }

    // ResetState::WaitingFlash: passo 2, igual a MSGMODE_NEW_CYCLE_1
    if (gSaveContext.fileNum != 0xFF && play->sramCtx.status != 0) {
        return;
    }
    SPDLOG_INFO("[Anchor] COOP_SONG_OF_TIME: voltando ao Dia 1");
    StartDawnTransition(play);
    sState = ResetState::None;
    sHasLocalReset = true;
    sLastLocalReset = Clock::now();
}
