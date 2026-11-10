#ifndef MELHORIA_H
#define MELHORIA_H

#include "jogo.h"

const char *nome_melhoria(TipoMelhoria t);
const char *descricao_melhoria(TipoMelhoria t);

/* Sorteia OFERTAS_MELHORIA melhorias diferentes (não oferece cura com a vida cheia). */
void sortear_melhorias(TipoMelhoria saida[OFERTAS_MELHORIA], const Personagem *mago);

void aplicar_melhoria(Personagem *mago, TipoMelhoria t);

#endif
