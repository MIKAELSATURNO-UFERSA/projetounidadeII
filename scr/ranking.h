#ifndef RANKING_H
#define RANKING_H

/*
 * ranking.c — a lógica da tela de ranking (o desenho fica em ui.c).
 * Trabalha numa CÓPIA do histórico, para poder ordenar por outros
 * critérios sem bagunçar o vetor principal (que fica ordenado por pontos).
 */

#include "jogo.h"

typedef struct {
    RegistroPartida   *vista;       /* cópia do histórico, na ordem escolhida */
    int                total;
    Criterio           criterio;
    AlgoritmoOrdenacao algoritmo;
    long               comparacoes_ordenacao;

    char  busca[TAM_NOME];          /* nome sendo digitado */
    int   buscou;                   /* 1 depois do primeiro ENTER */
    char  buscado[TAM_NOME];        /* nome da última busca feita */
    int   achou;
    int   partidas, vitorias, melhor_pontos, melhor_andar;
    long  comparacoes_binaria;
    long  comparacoes_linear;
} TelaRanking;

void abrir_ranking(TelaRanking *t, const Historico *h);
void fechar_ranking(TelaRanking *t);

/* Ordena a vista de novo com o critério e o algoritmo atuais. */
void reordenar_ranking(TelaRanking *t);

/* Procura t->busca com busca binária (recursiva) e com busca linear. */
void buscar_no_ranking(TelaRanking *t);

#endif
