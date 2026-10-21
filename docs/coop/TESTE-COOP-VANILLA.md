# Teste 2: campanha compartilhada (branch `coop-vanilla`)

O que este build acrescenta ao Anchor Beta, **só fora do Randomizer**:

| Recurso | Como funciona |
|---|---|
| Flags de história (`weekEventReg`) | Evento visto, NPC ajudado, máscara recebida: vale para os dois |
| Baús, interruptores, salas limpas, itens fixos do chão | Mesma sala: muda na hora. Outra sala: fica gravado e aparece quando o outro chegar lá |
| Itens permanentes | Máscaras, equipamentos, melhorias, canções, restos dos chefes, corações e peças de coração, mapas do Tingle |
| Itens de masmorra | Chaves pequenas, chave do chefe, mapa, bússola e fadas perdidas da masmorra em que quem pegou está. Chave gasta numa porta também desconta para o outro |
| Notificação | Aparece "<nome> compartilhou um item" com o ícone |

Continua **individual**: rupias, munição, corações e magia de recuperação, conteúdo de garrafa, itens de troca
(Moon's Tear, escrituras, cartas...) e tokens de Skulltula (dependem da casa em que cada um está e ficam para
uma próxima etapa).

Ainda **não** sincroniza: relógio e dia (cada um tem o seu), Canção do Tempo, minigames e cutscenes.

## Preparação

1. Baixe o `.zip` **2ship-windows** da última compilação verde do branch `coop-vanilla` no GitHub (aba Actions).
2. Extraia em `D:\mmcoop\Jogador1` e `D:\mmcoop\Jogador2` (pode sobrescrever). **Não** copie o `mm.o2r` de
   outra versão: se o jogo pedir a ROM, deixe extrair.
3. Abra `anchor-relay.exe`.
4. Nas duas janelas: Host `127.0.0.1`, porta `43383`, sala `Majoras bigode`, Team `default`.
5. No Jogador1, em Room Settings: **PvP Off** e **Sync Items & Flags LIGADO** (diferente do Teste 1).
6. **Saves novos e só de teste** nas duas janelas. Este build copia o save do time ao entrar.

## Casos de teste

Para cada caso, anote: resultado observado e, se falhar, o horário aproximado (para achar no log).

| # | Ações | Resultado esperado | Aprovado se |
|---|---|---|---|
| T2.1 | Jogador1 entra no jogo; depois Jogador2 entra | Jogador2 recebe "Save updated from team" | Os dois com o mesmo inventário |
| T2.2 | Jogador1 pega um item permanente (ex.: Deku Mask no início) | Jogador2 recebe o mesmo item e a notificação | Item aparece no menu do Jogador2 |
| T2.3 | Os dois na mesma sala com um baú; Jogador1 abre | O baú abre sozinho na tela do Jogador2 | Jogador2 não ganha o prêmio de novo |
| T2.4 | Jogador1 abre um baú; Jogador2 está em outra área e depois vai até lá | O baú já está aberto para o Jogador2 | Sem prêmio duplicado |
| T2.5 | Os dois na mesma sala; Jogador1 pisa num interruptor | A porta/efeito acontece também para o Jogador2 | Mesmo estado nas duas telas |
| T2.6 | Jogador1 pega uma peça de coração do chão com o Jogador2 na mesma sala | A peça some na tela do Jogador2 e os dois ganham 1 peça | Contagem de peças igual nas duas |
| T2.7 | Jogador1 pega rupias e munição | Nada muda para o Jogador2 | Carteiras diferentes |
| T2.8 | Jogador1 desconecta (Disable), pega um item, reconecta | Jogador2 recebe o item quando o Jogador1 volta | Item chega uma vez só |
| T2.9 | Os dois na mesma masmorra; Jogador1 pega uma chave pequena | O contador de chaves sobe nas duas telas | Mesmo número de chaves |
| T2.10 | Jogador1 abre uma porta trancada | A porta abre para os dois e a chave é descontada nas duas telas | Mesmo número de chaves |
| T2.11 | Jogar 10 minutos normalmente juntos | Nada trava, sem itens duplicados | Sem crash; logs sem erros de `[Anchor]` |

## Se algo der errado

Os logs ficam em `D:\mmcoop\Jogador1\logs` e `D:\mmcoop\Jogador2\logs`. Com o notebook ligado ao Claude,
basta dizer qual caso falhou e mais ou menos quando.
