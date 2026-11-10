#ifndef EQUIPE_H
#define EQUIPE_H

#include "jogo.h"
#include "assets.h"

/* ---- vetor dinâmico de ecos ---- */
void inicializar_equipe_ecos(EquipeEcos *eq);
void liberar_equipe_ecos(EquipeEcos *eq);
int  adicionar_eco(EquipeEcos *eq, Eco e);
int  remover_eco(EquipeEcos *eq, int indice);

/* A força do eco (vida, dano, velocidade) cresce com o andar. */
Eco  criar_eco(int andar, TipoEco tipo, Vector2 pos);

/* 1 se algum eco está a menos de "distancia" pixels do ponto p. */
int  eco_perto(const EquipeEcos *eq, Vector2 p, float distancia);

/*
 * Um quadro de inteligência dos ecos: andar, se afastar uns dos outros,
 * atirar e machucar no contato. Retorna o dano que o mago levou.
 * Conta em d->testes_separacao quantos pares de ecos foram comparados.
 */
int  atualizar_ecos(EquipeEcos *eq, Personagem *mago, const Mapa *m,
                    const MapaDistancias *md,
                    GerenciadorProjeteis *projeteis, GerenciadorEfeitos *efeitos,
                    Desempenho *d, float dt);

void desenhar_eco(const Eco *e, const Assets *a);

#endif
