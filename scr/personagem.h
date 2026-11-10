#ifndef PERSONAGEM_H
#define PERSONAGEM_H

#include "jogo.h"
#include "assets.h"

/*
 * O que o jogador quer fazer neste quadro.
 * Ler o teclado/mouse fica separado da lógica do mago: assim a lógica
 * pode ser testada sem janela, só passando Controles "inventados".
 */
typedef struct {
    Vector2 direcao;   /* para onde andar (tamanho 0 ou 1) */
    Vector2 alvo;      /* ponto da sala onde o mouse está */
    int     atirar;    /* botão esquerdo segurado */
    int     esquivar;  /* espaço apertado neste quadro */
} Controles;

Personagem criar_mago(Vector2 pos);
Controles  ler_controles(void);

void atualizar_mago(Personagem *p, Controles c, const Mapa *m,
                    GerenciadorProjeteis *projeteis, float dt);

/*
 * Aplica dano no mago vindo de "origem" (empurra para longe dela).
 * Retorna 1 se o dano entrou, 0 se ele estava invulnerável/esquivando.
 */
int danificar_mago(Personagem *p, int dano, Vector2 origem);

void desenhar_personagem(const Personagem *p, const Assets *a);

#endif
