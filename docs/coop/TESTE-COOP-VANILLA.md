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

Ainda **não** sincroniza: minigames e cutscenes. Relógio e Canção do Tempo existem como opções experimentais (Testes 3 e 4), desligadas por padrão.

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
| T2.8 | Jogador2 desconecta (Disable); Jogador1 pega um item; Jogador2 reconecta (Enable) | Jogador2 recebe "Save updated from team" e fica com o item | Item aparece uma vez só no Jogador2 |
| T2.9 | Os dois na mesma masmorra; Jogador1 pega uma chave pequena | O contador de chaves sobe nas duas telas | Mesmo número de chaves |
| T2.10 | Jogador1 abre uma porta trancada | A porta abre para os dois e a chave é descontada nas duas telas | Mesmo número de chaves |
| T2.11 | Jogar 10 minutos normalmente juntos | Nada trava, sem itens duplicados | Sem crash; logs sem erros de `[Anchor]` |

## Regra importante de reconexão

Quem **reconecta** recebe o save de quem **ficou conectado** (é assim que o Anchor funciona). Então, o que um
jogador fizer enquanto estiver desconectado **se perde** quando ele voltar. Para não perder progresso: só jogue
com o Anchor ligado.

## Se algo der errado

Os logs ficam em `D:\mmcoop\Jogador1\logs` e `D:\mmcoop\Jogador2\logs`. Com o notebook ligado ao Claude,
basta dizer qual caso falhou e mais ou menos quando.

## Teste 3 (opcional): relógio compartilhado experimental

Só faça depois do Teste 2. A opção fica em Network > Anchor, logo abaixo de "Current Room":
**Sincronizar relogio (experimental)**. Ligue nas **duas** janelas. Quem criou a sala (o "dono") manda na hora.

| # | Ações | Resultado esperado | Aprovado se |
|---|---|---|---|
| T3.1 | Os dois no Dia 1; o não dono fica parado num menu de pausa por 1 minuto real e depois fecha a pausa | Em até 2 s a hora do não dono pula para a hora do dono | Relógios iguais (diferença de poucos minutos do jogo) |
| T3.2 | Não dono entra num diálogo longo enquanto o dono anda | Nada muda durante o diálogo; ajusta logo depois | Sem salto no meio do texto |
| T3.3 | Só um dos dois liga a opção | Nada acontece | Relógios independentes |
| T3.4 | Um passa para o Dia 2 antes do outro | Aparece o aviso "voces estao em dias diferentes" | Nenhum dos dois tem a hora alterada |

## Teste 4 (opcional): Canção do Tempo compartilhada experimental

Ligue **Cancao do Tempo compartilhada (experimental)** nas duas janelas (mesmo lugar da opção do relógio).
Use saves de teste: este teste salva o fim de ciclo de verdade.

| # | Ações | Resultado esperado | Aprovado se |
|---|---|---|---|
| T4.1 | Jogador1 toca a Canção do Tempo e confirma "Sim" | Jogador2 vê o aviso "tocou a Cancao do Tempo"; em 30 s volta ao Dia 1 | Os dois no Dia 1, 6:00 |
| T4.2 | Igual ao T4.1, mas o Jogador2 clica em **Voltar ao Dia 1 agora** no menu do Anchor | Jogador2 volta na hora | Os dois no Dia 1 |
| T4.3 | Jogador2 está num diálogo quando o aviso chega | Espera o diálogo terminar e só então volta | Sem travamento |
| T4.4 | Jogador1 toca a Canção mas responde "Não" | Nada acontece com o Jogador2 | Jogador2 continua no mesmo dia |
| T4.5 | Depois de voltar, conferir itens e flags | Itens permanentes e máscaras mantidos; rupias, munição e baús do ciclo zerados como no jogo normal | Mesmo estado nas duas janelas |

Ainda não tratado: Canção do Tempo Invertida e Canção de Tempo Dupla, e dias diferentes no relógio.
