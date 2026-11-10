# O Último Andar

Jogo em C com [raylib](https://www.raylib.com/), feito para a disciplina de Estruturas de Dados.
Um mago sobe uma torre de cinco andares enfrentando ondas de Ecos, em tempo real.
Ao limpar um andar, a escada se abre e o mago escolhe uma melhoria antes de subir.
Toda partida fica guardada: há ranking, diário de combate, e quem cai num andar
deixa ali o seu "eco" para assombrar as próximas partidas.

## Como compilar

Compile **todos** os arquivos `.c` da pasta `scr` (a cada fase entram arquivos novos,
então use sempre `scr/*.c`).

**Windows (MinGW / w64devkit com raylib instalada):**

```
gcc scr/*.c -o ultimo-andar.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

Se a raylib estiver numa pasta própria, acrescente `-I<pasta>/include -L<pasta>/lib`.

**Linux:**

```
gcc scr/*.c -o ultimo-andar -lraylib -lm
```

Rode o jogo **a partir da pasta do projeto** (onde fica a pasta `assets`), porque os
sprites, os andares e o save são procurados em `assets/...`.

## Os andares (`assets/andares/andarN.txt`)

Cada andar é um arquivo de texto que pode ser editado no Bloco de Notas:

```
// linhas começando com // são comentários
nome: O Salão das Colunas
ondas: 2
mapa:
####################
#.................S#
#..#...#....#...#..#
...                       (13 linhas de 20 caracteres)
```

No mapa: `#` parede, `.` chão, `M` onde o mago começa e `S` onde fica a escada
(exatamente um de cada). Ao carregar, o jogo confere: tamanho de cada linha,
caracteres válidos, borda fechada, um `M` e um `S`, e — com um *flood fill* recursivo —
se todo o chão é alcançável a partir do `M`. Se algo estiver errado, o jogo mostra
o motivo e a linha do problema no topo da sala e usa um mapa padrão no lugar.
Com o modo debug ligado, **F3** relê o arquivo do andar atual.

## Os dados salvos (`assets/dados/`)

A pasta é criada sozinha na primeira partida.

| Arquivo | Tipo | O que guarda |
|---|---|---|
| `historico.bin` | binário | Um `RegistroPartida` por partida (nome, andar, tempo, pontos...), sempre acrescentado no fim. Ao abrir o jogo, o arquivo inteiro é lido para um vetor; registros cortados ou com valores impossíveis são descartados com aviso no menu. |
| `diario.txt` | texto | Diário de combate: cada acontecimento vira uma linha. No fim da partida, o jogo lê o arquivo até o fim (EOF) e mostra as últimas linhas usando uma fila circular. Passando de 256 KB, vira `diario_antigo.txt`. |
| `jogador.txt` | texto | O último nome digitado, para já aparecer escrito na próxima vez. |

**Ecos lembrança:** ao começar cada andar, o jogo separa as derrotas do histórico,
ordena por andar (Quick Sort com pivô aleatório) e procura com busca binária alguém
que caiu naquele andar. Se achar, um eco dourado com o nome dessa pessoa aparece
junto da primeira onda (derrotá-lo vale +100 pontos).

## Ranking (tecla R no menu)

- **Setas esquerda/direita:** ordena por pontos, andar, nome ou mais recentes.
- **TAB:** alterna entre Merge Sort e Quick Sort (a tela mostra quantas comparações cada um fez).
- **Digitar um nome + ENTER:** busca o jogador com busca binária recursiva (numa cópia
  ordenada por nome) e com busca linear, mostrando as comparações de cada uma.
- **ESC:** volta ao menu.

## Controles

| Tecla | Ação |
|---|---|
| WASD ou setas | mover |
| Mouse | mirar |
| Clique esquerdo (segurar) | lançar esferas |
| Espaço | esquiva (fica invulnerável e atravessa tiros por um instante) |
| P | pausar |
| F1 | modo debug (desempenho dos algoritmos, círculos de colisão e mapa de distâncias) |
| F2 (com debug ligado) | +10 ecos, para testar desempenho |
| F3 (com debug ligado) | relê o arquivo do andar atual |
| 1, 2, 3 ou clique | escolher a melhoria entre andares |
| R (no menu) | ranking |
| ESC | sair (nas telas de nome e ranking, volta ao menu) |

## Organização do código

| Arquivo | O que faz |
|---|---|
| `jogo.h` | Todas as estruturas de dados do jogo |
| `vetor.h` | Contas com vetores 2D (somar, normalizar, distância) |
| `main.c` | Janela, troca de telas e ordem de desenho |
| `partida.c` | Regras do jogo a cada quadro (andares, ondas, escada, vitória, derrota) |
| `andar.c` | Lê e valida os arquivos dos andares (inclui o flood fill recursivo) |
| `melhoria.c` | Melhorias entre andares (sorteio e efeito) |
| `mapa.c` | Colisão com paredes, linha de visão e mapa de distâncias (BFS) |
| `personagem.c` | Controles e movimento do mago, esquiva, dano |
| `equipe.c` | Vetor dinâmico de ecos e o comportamento deles |
| `projetil.c` | Esferas do mago e tiros dos ecos |
| `ordenacao.c` | Insertion Sort dos ecos por Y (ordem de desenho); Merge Sort e Quick Sort do histórico |
| `onda.c` | Quantos ecos nascem em cada onda e onde |
| `historico.c` | Histórico em arquivo binário, inserção ordenada, pontos e estatísticas |
| `diario.c` | Diário de combate em texto (append e leitura até o EOF com fila circular) |
| `ranking.c` | Lógica da tela de ranking (reordenar e buscar) |
| `busca.c` | Busca binária recursiva por nome, busca binária por andar e busca linear |
| `efeitos.c` | Números de dano e impactos |
| `ui.c` | HUD, rodapé e telas (menu, nome, ranking, pausa, melhorias, vitória, derrota, debug) |
| `assets.c` | Carrega os sprites |

## Andamento

- [x] **Fase 1 – Tempo real:** movimento livre, mira com mouse, recargas, esquiva, dano por contato com invulnerabilidade, ecos caçador e atirador, Insertion Sort para a ordem de desenho
- [x] **Fase 2 – A torre:** andares lidos de arquivos `.txt` (validação + flood fill recursivo), escada, melhorias entre andares
- [x] **Fase 3 – A memória:** ranking e histórico em arquivo binário, diário em texto, Merge/Quick Sort, busca binária, ecos lembrança
- [ ] **Fase 4 – Fechamento:** revisão final e ajustes
