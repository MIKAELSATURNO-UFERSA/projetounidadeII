#include "ranking.h"
#include "ordenacao.h"
#include "busca.h"
#include <stdlib.h>
#include <string.h>

void abrir_ranking(TelaRanking *t, const Historico *h) {
    memset(t, 0, sizeof(*t));
    t->criterio  = CRITERIO_PONTOS;
    t->algoritmo = ALGORITMO_MERGE;

    if (h->total > 0) {
        t->vista = (RegistroPartida *) malloc(h->total * sizeof(RegistroPartida));
        if (t->vista != NULL) {
            memcpy(t->vista, h->itens, h->total * sizeof(RegistroPartida));
            t->total = h->total;
        }
    }
    reordenar_ranking(t);
}

void fechar_ranking(TelaRanking *t) {
    free(t->vista);
    t->vista = NULL;
    t->total = 0;
}

void reordenar_ranking(TelaRanking *t) {
    t->comparacoes_ordenacao = 0;
    ordenar_registros(t->vista, t->total, t->criterio, t->algoritmo, &t->comparacoes_ordenacao);
}

void buscar_no_ranking(TelaRanking *t) {
    t->buscou = 1;
    t->achou = 0;
    t->partidas = t->vitorias = t->melhor_pontos = t->melhor_andar = 0;
    t->comparacoes_binaria = 0;
    t->comparacoes_linear  = 0;
    strncpy(t->buscado, t->busca, TAM_NOME - 1);
    t->buscado[TAM_NOME - 1] = '\0';

    if (t->total == 0 || t->buscado[0] == '\0') return;

    /* Busca linear: funciona na vista como ela estiver, sem ordenar. */
    busca_linear_nome(t->vista, t->total, t->buscado, &t->comparacoes_linear);

    /* Busca binária: precisa de uma cópia ordenada por nome. */
    RegistroPartida *por_nome = (RegistroPartida *) malloc(t->total * sizeof(RegistroPartida));
    if (por_nome == NULL) return;
    memcpy(por_nome, t->vista, t->total * sizeof(RegistroPartida));
    merge_sort_registros(por_nome, t->total, CRITERIO_NOME, NULL);

    int achado = busca_binaria_nome(por_nome, 0, t->total - 1, t->buscado, &t->comparacoes_binaria);

    if (achado >= 0) {
        /* Ordenado por nome, todas as partidas dessa pessoa estão lado a lado:
           volta até a primeira e anda até a última. */
        int ini = achado, fim = achado;
        while (ini > 0 && comparar_nomes(por_nome[ini - 1].nome, t->buscado) == 0) ini--;
        while (fim < t->total - 1 && comparar_nomes(por_nome[fim + 1].nome, t->buscado) == 0) fim++;

        t->achou = 1;
        for (int i = ini; i <= fim; i++) {
            t->partidas++;
            t->vitorias += por_nome[i].venceu;
            if (por_nome[i].pontos > t->melhor_pontos) t->melhor_pontos = por_nome[i].pontos;
            if (por_nome[i].andar  > t->melhor_andar)  t->melhor_andar  = por_nome[i].andar;
        }
    }
    free(por_nome);
}
