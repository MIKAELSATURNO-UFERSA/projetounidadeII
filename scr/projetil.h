#ifndef PROJETIL_H
#define PROJETIL_H

#include "jogo.h"

void inicializar_projeteis(GerenciadorProjeteis *g);

/* Procura uma vaga livre no vetor e cria o projétil nela. */
void disparar_projetil(GerenciadorProjeteis *g, Vector2 pos, Vector2 vel,
                       float raio, int dano, int do_jogador, float duracao);

/*
 * Move todos os projéteis e resolve as batidas:
 *  - esfera do mago acerta ecos (ecos com vida <= 0 são removidos)
 *  - tiro de eco acerta o mago (a esquiva atravessa os tiros)
 * Retorna o dano que o mago levou e soma em *ecos_destruidos
 * quantos ecos morreram neste quadro (e em *lembrancas_destruidas
 * quantos deles eram ecos lembrança).
 */
int atualizar_projeteis(GerenciadorProjeteis *g, const Mapa *m,
                        Personagem *mago, EquipeEcos *ecos,
                        GerenciadorEfeitos *efeitos, float dt,
                        int *ecos_destruidos, int *lembrancas_destruidas);

int  contar_projeteis_ativos(const GerenciadorProjeteis *g);
void desenhar_projeteis(const GerenciadorProjeteis *g);

#endif
