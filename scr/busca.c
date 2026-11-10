#include "busca.h"
#include "ordenacao.h"
#include <stddef.h>

int busca_linear_nome(const RegistroPartida *v, int n, const char *nome, long *comparacoes) {
    for (int i = 0; i < n; i++) {
        if (comparacoes != NULL) (*comparacoes)++;
        if (comparar_nomes(v[i].nome, nome) == 0) return i;
    }
    return -1;      /* olhou todos os n: no pior caso, O(n) */
}

int busca_binaria_nome(const RegistroPartida *v, int ini, int fim, const char *nome,
                       long *comparacoes) {
    /* Cada chamada joga fora metade do intervalo: no máximo ~log2(n) chamadas. */

    /* Caso base: o intervalo ficou vazio, então o nome não está no vetor. */
    if (ini > fim) return -1;

    int meio = ini + (fim - ini) / 2;
    if (comparacoes != NULL) (*comparacoes)++;
    int r = comparar_nomes(nome, v[meio].nome);

    if (r == 0) return meio;                                              /* achou */
    if (r < 0)  return busca_binaria_nome(v, ini, meio - 1, nome, comparacoes); /* metade esquerda */
    return busca_binaria_nome(v, meio + 1, fim, nome, comparacoes);       /* metade direita */
}

int busca_binaria_andar(const RegistroPartida *v, int n, int andar, long *comparacoes) {
    int ini = 0, fim = n - 1;

    while (ini <= fim) {
        int meio = ini + (fim - ini) / 2;
        if (comparacoes != NULL) (*comparacoes)++;

        if (v[meio].andar == andar) return meio;
        /* ordem DECRESCENTE: andares maiores ficam à esquerda */
        if (v[meio].andar > andar) ini = meio + 1;
        else                       fim = meio - 1;
    }
    return -1;
}
