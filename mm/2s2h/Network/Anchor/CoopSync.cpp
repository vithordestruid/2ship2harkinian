#include "CoopSync.h"

#include <spdlog/spdlog.h>

#include "2s2h/BenGui/Notification.h"
#include "2s2h/Rando/Rando.h"

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64actor.h"
#include "z64interface.h"
#include "z64play.h"
extern PlayState* gPlayState;
}

// MM Coop (campanha vanilla): veja CoopSync.h para o fluxo geral.

namespace {

bool sApplyingRemote = false;

// Liga sApplyingRemote enquanto existir no escopo, mesmo se algo lancar excecao.
struct ApplyingRemoteScope {
    ApplyingRemoteScope() {
        sApplyingRemote = true;
    }
    ~ApplyingRemoteScope() {
        sApplyingRemote = false;
    }
};

// Limites das flags de cena (ver ActorContextSceneFlags e CycleSceneFlags em z64actor.h/z64save.h):
// switches 0x00-0x3F sao gravados no save do ciclo; 0x40-0x7F sao temporarios da cena.
// collectible 0x01-0x1F sao gravados; 0x20-0x7F sao temporarios.
constexpr s32 kSwitchSavedMax = 0x40;
constexpr s32 kSwitchMax = 0x80;
constexpr s32 kCollectibleSavedMax = 0x20;
constexpr s32 kCollectibleMax = 0x80;
constexpr s32 kBitsPerWord = 32;

bool IsSaveReady() {
    return gPlayState != nullptr && gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 &&
           gSaveContext.gameMode == GAMEMODE_NORMAL;
}

bool IsValidSceneId(s16 sceneId) {
    return sceneId >= 0 && sceneId < SCENE_MAX;
}

// Mesma escolha de indice que Play_SaveCycleSceneFlags / Actor_InitContext:
// a maioria das flags usa a cena "original" (ex.: vila da primavera usa a do inverno),
// mas collectible e clearedRoom do Templo de Stone Tower invertido usam o proprio indice.
CycleSceneFlags* GetCycleFlags(s16 sceneId, FlagType flagType) {
    s16 index = Play_GetOriginalSceneId(sceneId);
    if (sceneId == SCENE_INISIE_R &&
        (flagType == FLAG_CYCL_SCENE_COLLECTIBLE || flagType == FLAG_CYCL_SCENE_CLEARED_ROOM)) {
        index = sceneId;
    }
    if (index < 0 || index >= (s16)ARRAY_COUNT(gSaveContext.cycleSceneFlags)) {
        return nullptr;
    }
    return &gSaveContext.cycleSceneFlags[index];
}

void SetBit(u32* word, s32 bit, bool set) {
    if (set) {
        *word |= (1u << bit);
    } else {
        *word &= ~(1u << bit);
    }
}

bool ApplyGlobalFlag(FlagType flagType, s32 flag, bool set) {
    if (flagType != FLAG_WEEK_EVENT_REG) {
        return false;
    }
    // weekEventReg: byte << 8 | mascara do bit. 100 bytes no save.
    s32 byteIndex = flag >> 8;
    s32 mask = flag & 0xFF;
    if (byteIndex < 0 || byteIndex >= (s32)ARRAY_COUNT(gSaveContext.save.saveInfo.weekEventReg) || mask == 0) {
        return false;
    }
    if (set) {
        Flags_SetWeekEventReg(flag);
    } else {
        Flags_ClearWeekEventReg(flag);
    }
    return true;
}

// Cena em que o jogador local esta agora: muda as flags "vivas" (actorCtx), que o jogo grava no save
// ao sair da cena. Assim nada se perde quando o jogador local deixa a cena depois.
bool ApplyLiveSceneFlag(FlagType flagType, s32 flag, bool set) {
    PlayState* play = gPlayState;
    switch (flagType) {
        case FLAG_CYCL_SCENE_SWITCH:
            if (flag < 0 || flag >= kSwitchMax) {
                return false;
            }
            set ? Flags_SetSwitch(play, flag) : Flags_UnsetSwitch(play, flag);
            return true;
        case FLAG_CYCL_SCENE_CHEST:
            if (flag < 0 || flag >= kBitsPerWord || !set) {
                return false; // o jogo nao tem "fechar bau"
            }
            Flags_SetTreasure(play, flag);
            return true;
        case FLAG_CYCL_SCENE_CLEARED_ROOM:
            if (flag < 0 || flag >= kBitsPerWord) {
                return false;
            }
            set ? Flags_SetClear(play, flag) : Flags_UnsetClear(play, flag);
            return true;
        case FLAG_CYCL_SCENE_COLLECTIBLE:
            if (flag <= 0 || flag >= kCollectibleMax || !set) {
                return false;
            }
            Flags_SetCollectible(play, flag);
            return true;
        default:
            return false;
    }
}

// Outra cena: muda direto o save do ciclo. Flags temporarias de outra cena sao descartadas,
// porque o proprio jogo as apaga ao trocar de cena.
bool ApplySavedSceneFlag(s16 sceneId, FlagType flagType, s32 flag, bool set) {
    CycleSceneFlags* cycle = GetCycleFlags(sceneId, flagType);
    if (cycle == nullptr) {
        return false;
    }
    switch (flagType) {
        case FLAG_CYCL_SCENE_SWITCH:
            if (flag < 0 || flag >= kSwitchSavedMax) {
                return false;
            }
            SetBit(flag < kBitsPerWord ? &cycle->switch0 : &cycle->switch1, flag % kBitsPerWord, set);
            return true;
        case FLAG_CYCL_SCENE_CHEST:
            if (flag < 0 || flag >= kBitsPerWord || !set) {
                return false;
            }
            SetBit(&cycle->chest, flag, true);
            return true;
        case FLAG_CYCL_SCENE_CLEARED_ROOM:
            if (flag < 0 || flag >= kBitsPerWord) {
                return false;
            }
            SetBit(&cycle->clearedRoom, flag, set);
            return true;
        case FLAG_CYCL_SCENE_COLLECTIBLE:
            if (flag <= 0 || flag >= kCollectibleSavedMax || !set) {
                return false;
            }
            SetBit(&cycle->collectible, flag, true);
            return true;
        default:
            return false;
    }
}

} // namespace

bool CoopSync_IsApplyingRemote() {
    return sApplyingRemote;
}

bool CoopSync_ShouldSendFlag(FlagType flagType, u32 flag) {
    if (sApplyingRemote || IS_RANDO) {
        return false;
    }
    // FLAG_EVENT_INF: estados temporarios da sessao (minigames, cutscenes em andamento) -> individual.
    // FLAG_WEEK_EVENT_REG_HORSE_RACE: estado da corrida em andamento -> individual.
    // FLAG_RANDO_INF: so existe no Randomizer, que tem sincronia propria.
    return flagType == FLAG_WEEK_EVENT_REG;
}

bool CoopSync_ShouldSendSceneFlag(s16 sceneId, FlagType flagType, u32 flag) {
    if (sApplyingRemote || IS_RANDO || !IsValidSceneId(sceneId)) {
        return false;
    }
    switch (flagType) {
        case FLAG_CYCL_SCENE_SWITCH:
        case FLAG_CYCL_SCENE_CHEST:
        case FLAG_CYCL_SCENE_CLEARED_ROOM:
        case FLAG_CYCL_SCENE_COLLECTIBLE:
            return true;
        default:
            return false;
    }
}

bool CoopSync_ShouldShareItem(u8 item) {
    switch (item) {
        // Equipamentos e itens de inventario permanentes
        case ITEM_OCARINA_OF_TIME:
        case ITEM_BOW:
        case ITEM_ARROW_FIRE:
        case ITEM_ARROW_ICE:
        case ITEM_ARROW_LIGHT:
        case ITEM_PICTOGRAPH_BOX:
        case ITEM_LENS_OF_TRUTH:
        case ITEM_HOOKSHOT:
        case ITEM_SWORD_GREAT_FAIRY:
        case ITEM_BOTTLE:
        case ITEM_SWORD_KOKIRI:
        case ITEM_SWORD_RAZOR:
        case ITEM_SWORD_GILDED:
        case ITEM_SHIELD_HERO:
        case ITEM_SHIELD_MIRROR:
        // Melhorias de capacidade
        case ITEM_QUIVER_30:
        case ITEM_QUIVER_40:
        case ITEM_QUIVER_50:
        case ITEM_BOMB_BAG_20:
        case ITEM_BOMB_BAG_30:
        case ITEM_BOMB_BAG_40:
        case ITEM_WALLET_ADULT:
        case ITEM_WALLET_GIANT:
        case ITEM_DEKU_STICK_UPGRADE_20:
        case ITEM_DEKU_STICK_UPGRADE_30:
        case ITEM_DEKU_NUT_UPGRADE_30:
        case ITEM_DEKU_NUT_UPGRADE_40:
        // Itens de missao principal
        case ITEM_REMAINS_ODOLWA:
        case ITEM_REMAINS_GOHT:
        case ITEM_REMAINS_GYORG:
        case ITEM_REMAINS_TWINMOLD:
        case ITEM_SONG_SONATA:
        case ITEM_SONG_LULLABY:
        case ITEM_SONG_LULLABY_INTRO:
        case ITEM_SONG_NOVA:
        case ITEM_SONG_ELEGY:
        case ITEM_SONG_OATH:
        case ITEM_SONG_TIME:
        case ITEM_SONG_HEALING:
        case ITEM_SONG_EPONA:
        case ITEM_SONG_SOARING:
        case ITEM_SONG_STORMS:
        case ITEM_BOMBERS_NOTEBOOK:
        case ITEM_HEART_CONTAINER:
        case ITEM_HEART_PIECE:
        case ITEM_HEART_PIECE_2:
            return true;
        default:
            break;
    }
    // Todas as mascaras (Deku ate Gigante)
    if (item >= ITEM_MASK_DEKU && item <= ITEM_MASK_GIANT) {
        return true;
    }
    // Pontos do mapa do Tingle
    if (item >= ITEM_MAP_POINT_GREAT_BAY && item <= ITEM_MAP_POINT_ZORA_CAPE) {
        return true;
    }
    // Ficam individuais: rupias, municao, coracoes e magia de recuperacao, conteudo de garrafa,
    // itens de troca (Moon's Tear, escrituras, cartas...) e itens que dependem da cena atual do jogo
    // local (chaves, mapa, bussola, fadas perdidas, tokens de Skulltula): o Item_Give credita esses
    // na masmorra/casa em que o jogador local estiver, que pode nao ser a de quem pegou.
    return false;
}

bool CoopSync_ApplyFlag(s16 sceneId, FlagType flagType, s32 flag, bool set) {
    if (!IsSaveReady() || IS_RANDO) {
        return false;
    }

    ApplyingRemoteScope scope;

    if (sceneId == SCENE_MAX) {
        // So aceita os tipos globais que tambem enviamos (hoje: weekEventReg).
        return ApplyGlobalFlag(flagType, flag, set);
    }

    if (!IsValidSceneId(sceneId)) {
        return false;
    }

    if (gPlayState->sceneId == sceneId) {
        return ApplyLiveSceneFlag(flagType, flag, set);
    }
    return ApplySavedSceneFlag(sceneId, flagType, flag, set);
}

bool CoopSync_ApplyItem(u8 item) {
    if (!IsSaveReady() || IS_RANDO || !CoopSync_ShouldShareItem(item)) {
        return false;
    }

    ApplyingRemoteScope scope;
    Item_Give(gPlayState, item);
    return true;
}
