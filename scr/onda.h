#ifndef ONDA_H
#define ONDA_H

#include "jogo.h"

/* Quantos ecos (e quantos deles atiradores) nascem na onda "onda" do andar "andar". */
int  ecos_para_onda(int andar, int onda);
int  atiradores_na_onda(int andar, int onda);

/* Cria os ecos da onda em lugares livres, longe do mago. A força deles depende do andar. */
void spawnar_onda(EquipeEcos *ecos, int andar, int onda, const Mapa *m, const Personagem *mago);

/* Cria "quantidade" ecos extras (usado pelo modo de teste de desempenho, F2). */
void spawnar_ecos_extras(EquipeEcos *ecos, int quantidade, int andar,
                         const Mapa *m, const Personagem *mago);

#endif
