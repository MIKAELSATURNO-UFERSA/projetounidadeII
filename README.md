# O Último Andar

Jogo em C com [raylib](https://www.raylib.com/), feito para a disciplina de Estruturas de Dados.
Um mago sobe uma torre de cinco andares enfrentando ondas de Ecos, em tempo real.
Ao limpar um andar, a escada se abre e o mago escolhe uma melhoria antes de subir.
Toda partida fica guardada: há ranking, diário de combate, e quem cai num andar
deixa ali o seu "eco" para assombrar as próximas partidas.

## Como compilar



gcc scr/*.c -o ultimo-andar.exe -lraylib -lopengl32 -lgdi32 -lwinmm








**Ecos lembrança:** ao começar cada andar, o jogo separa as derrotas do histórico,
ordena por andar (Quick Sort com pivô aleatório) e procura com busca binária alguém
que caiu naquele andar. Se achar, um eco dourado com o nome dessa pessoa aparece
junto da primeira onda (derrotá-lo vale +100 pontos).


## Controles

| Tecla | Ação |
|---|---|
| WASD ou setas | mover |
| Mouse | mirar |
| Clique esquerdo (segurar) | lançar esferas |
| Espaço | esquiva (fica invulnerável e atravessa tiros por um instante) |
| P | pausar |
| 1, 2, 3 ou clique | escolher a melhoria entre andares |
| R (no menu) | ranking |
| ESC | sair (nas telas de nome e ranking, volta ao menu) |



## Andamento

- [x] **Fase 1 – Tempo real:** movimento livre, mira com mouse, recargas, esquiva, dano por contato com invulnerabilidade, ecos caçador e atirador, Insertion Sort para a ordem de desenho
- [x] **Fase 2 – A torre:** andares lidos de arquivos `.txt` (validação + flood fill recursivo), escada, melhorias entre andares
- [x] **Fase 3 – A memória:** ranking e histórico em arquivo binário, diário em texto, Merge/Quick Sort, busca binária, ecos lembrança
- [ ] **Fase 4 – Fechamento:** revisão final e ajustes
