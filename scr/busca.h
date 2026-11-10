#ifndef BUSCA_H
#define BUSCA_H

/*
 * Buscas no histórico. Todas somam em *comparacoes quantas comparações
 * fizeram (pode ser NULL), para dar para comparar os métodos.
 */

#include "jogo.h"

/*
 * Busca linear: olha um por um, do começo ao fim. Funciona em qualquer
 * ordem. Retorna o índice do primeiro registro com esse nome, ou -1.
 */
int busca_linear_nome(const RegistroPartida *v, int n, const char *nome, long *comparacoes);

/*
 * Busca binária RECURSIVA por nome. O vetor precisa estar ordenado por
 * CRITERIO_NOME. Procura entre os índices ini e fim (inclusive).
 * Retorna o índice de algum registro com esse nome, ou -1.
 */
int busca_binaria_nome(const RegistroPartida *v, int ini, int fim, const char *nome,
                       long *comparacoes);

/*
 * Busca binária (iterativa) por andar. O vetor precisa estar ordenado por
 * CRITERIO_ANDAR (andar em ordem DECRESCENTE). Retorna um índice com esse
 * andar, ou -1.
 */
int busca_binaria_andar(const RegistroPartida *v, int n, int andar, long *comparacoes);

#endif
