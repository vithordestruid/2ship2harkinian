# MM Coop Online

Coop online para dois jogadores na campanha original de Majora's Mask, sem Randomizer, em cima do
2 Ship 2 Harkinian e do Anchor (coop criado por garrettjoecox).

## Branches deste repositório

| Branch | O que é | Situação |
|---|---|---|
| `develop` | 2S2H original, sem mudanças | Intacto |
| `anchor-beta` | Anchor Beta do garrettjoecox + compilação na nuvem só para Windows | Testado: os dois se veem (21/10/2026) |
| `coop-vanilla` | `anchor-beta` + campanha compartilhada (flags, itens, masmorras) + relógio experimental | Compila; **ainda não testado jogando** |

## Como baixar um build

1. Aba **Actions**, escolha a execução mais recente **verde** do branch desejado.
2. No fim da página, em **Artifacts**, baixe **2ship-windows**.
3. Extraia numa pasta própria. Use a sua ROM NTSC-U 1.0 quando o jogo pedir. Não reaproveite `mm.o2r` de outra versão.

## Servidor

O jogo conecta num servidor relay do Anchor (`garrettjoecox/anchor`). Para jogar pela internet sem abrir portas no
roteador, os dois PCs podem usar o Tailscale e apontar o Host para o IP do PC que roda o relay.

## Testes

Veja [TESTE-COOP-VANILLA.md](TESTE-COOP-VANILLA.md).

## O que muda no código (branch `coop-vanilla`)

| Arquivo | Mudança |
|---|---|
| `mm/2s2h/Network/Anchor/CoopSync.h/.cpp` | **Novo.** Regras do que compartilhar e como aplicar no outro jogo |
| `mm/2s2h/Network/Anchor/Packets/CoopTime.cpp` | **Novo.** Relógio experimental |
| `mm/2s2h/Network/Anchor/Anchor.cpp` | Ganchos de flags, itens, item do chão e envio do relógio |
| `mm/2s2h/Network/Anchor/Packets/SetFlag.cpp`, `UnsetFlag.cpp` | Aplicam as flags recebidas (estavam desligadas no Anchor Beta) |
| `mm/2s2h/Network/Anchor/Packets/GiveItem.cpp` | Item vanilla além do item de Randomizer |
| `mm/2s2h/Network/Anchor/Packets/UpdateDungeonItems.cpp` | Validação do índice e envio só dentro de masmorra |
| `mm/2s2h/Network/Anchor/Packets/UpdateRoomState.cpp`, `Menu.cpp` | PvP desligado por padrão; opção do relógio |
| `.github/workflows/main.yml` | Compila só Windows e mantém cache (branch `anchor-beta`) |
