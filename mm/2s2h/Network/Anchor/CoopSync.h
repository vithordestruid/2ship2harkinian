#ifndef NETWORK_ANCHOR_COOP_SYNC_H
#define NETWORK_ANCHOR_COOP_SYNC_H
#ifdef __cplusplus

#include <cstdint>

#include "2s2h/GameInteractor/GameInteractor.h"

extern "C" {
#include "z64.h"
}

/**
 * MM Coop (campanha vanilla): regras de sincronia de flags e itens fora do Randomizer.
 *
 * Fluxo:
 *  - Ganchos OnFlagSet/OnSceneFlagSet/OnItemGive (Anchor::RegisterHooks) perguntam a estas funcoes
 *    se a mudanca deve ir para a rede.
 *  - Ao receber um pacote, CoopSync_ApplyFlag / CoopSync_ApplyItem aplicam a mudanca localmente.
 *  - Enquanto uma mudanca remota esta sendo aplicada, CoopSync_IsApplyingRemote() fica verdadeiro
 *    para os ganchos nao reenviarem a mesma mudanca (evita eco infinito entre os jogadores).
 */

// Verdadeiro enquanto uma mudanca recebida pela rede esta sendo aplicada.
bool CoopSync_IsApplyingRemote();

// Flags globais (sceneId ignorado) e de cena que devem ser compartilhadas.
bool CoopSync_ShouldSendFlag(FlagType flagType, u32 flag);
bool CoopSync_ShouldSendSceneFlag(s16 sceneId, FlagType flagType, u32 flag);

// Itens que devem ser compartilhados na campanha vanilla.
bool CoopSync_ShouldShareItem(u8 item);

// Aplica uma flag recebida. sceneId == SCENE_MAX indica flag global (weekEventReg).
// Retorna falso se o pacote foi ignorado (tipo nao suportado ou valores fora da faixa).
bool CoopSync_ApplyFlag(s16 sceneId, FlagType flagType, s32 flag, bool set);

// Verdadeiro se este item do chao (EnItem00) foi coletado pelo outro jogador nesta cena.
// Usado para fazer o item sumir e evitar que seja pego duas vezes.
bool CoopSync_WasCollectedRemotely(s16 sceneId, s32 collectibleFlag);

// Chamado ao entrar numa cena: descarta a lista de itens do chao da cena anterior
// (depois da Cancao do Tempo esses itens voltam a existir e nao podem sumir por engano).
void CoopSync_OnSceneChange();

// Aplica um item recebido. Retorna falso se o item nao e compartilhavel ou o jogo nao esta pronto.
bool CoopSync_ApplyItem(u8 item);

#endif // __cplusplus
#endif // NETWORK_ANCHOR_COOP_SYNC_H
