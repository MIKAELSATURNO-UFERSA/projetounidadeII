#ifndef ORDENACAO_H
#define ORDENACAO_H

/*
 * Algoritmos de ordenação do jogo.
 *  - Insertion Sort: ecos por Y, a cada quadro (ordem de desenho)
 *  - Merge Sort e Quick Sort: registros do histórico (ranking)
 */

#include "jogo.h"

/*
 * Ordena o vetor de ecos pelo Y (de cima para baixo da tela).
 * Retorna quantas comparações fez, para medir o custo no modo debug.
 */
int insertion_sort_ecos_por_y(Eco *v, int n);

/*
 * Compara dois registros pelo critério escolhido.
 * Retorna < 0 se "a" vem antes de "b", > 0 se vem depois e 0 se empatam.
 */
int comparar_registros(const RegistroPartida *a, const RegistroPartida *b, Criterio c);

/* Compara nomes sem diferenciar maiúsculas de minúsculas ("mikael" == "Mikael"). */
int comparar_nomes(const char *a, const char *b);

/* Os dois somam em *comparacoes quantas comparações fizeram (pode ser NULL). */
void merge_sort_registros(RegistroPartida *v, int n, Criterio c, long *comparacoes);
void quick_sort_registros(RegistroPartida *v, int n, Criterio c, long *comparacoes);
void ordenar_registros(RegistroPartida *v, int n, Criterio c,
                       AlgoritmoOrdenacao alg, long *comparacoes);

const char *nome_criterio(Criterio c);
const char *nome_algoritmo(AlgoritmoOrdenacao a);

#endif
