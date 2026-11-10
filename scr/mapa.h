#ifndef MAPA_H
#define MAPA_H

#include "jogo.h"

/* Os mapas vêm dos arquivos de andar (veja andar.h). */

void desenhar_mapa(const Mapa *m);

/* Fora do mapa também conta como parede. */
int eh_parede(const Mapa *m, int tx, int ty);

/* Centro (em pixels) de um tile. */
Vector2 centro_do_tile(int tx, int ty);

/* 1 se um círculo nessa posição encosta em alguma parede. */
int circulo_colide_parede(const Mapa *m, Vector2 centro, float raio);

/*
 * Move um círculo pelo deslocamento pedido, sem atravessar paredes.
 * Os eixos são tratados separadamente: se bater de frente, ainda
 * desliza ao longo da parede. Retorna 1 se bateu em algo.
 */
int mover_com_colisao(const Mapa *m, Vector2 *pos, Vector2 deslocamento, float raio);

/* 1 se não há parede no caminho reto entre a e b. */
int linha_de_visao(const Mapa *m, Vector2 a, Vector2 b);

/* Como linha_de_visao, mas para um círculo de raio "raio" passar inteiro. */
int caminho_livre(const Mapa *m, Vector2 a, Vector2 b, float raio);

/* Preenche "md" com a distância (em tiles) de cada tile até o ponto "alvo". */
void calcular_distancias(const Mapa *m, Vector2 alvo, MapaDistancias *md);

/*
 * Direção (tamanho 1) para o tile vizinho mais perto do alvo.
 * Retorna (0,0) se já está no tile do alvo ou se não há caminho.
 */
Vector2 direcao_pelo_mapa(const Mapa *m, const MapaDistancias *md, Vector2 pos);

#endif
