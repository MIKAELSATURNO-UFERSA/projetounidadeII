#ifndef HISTORICO_H
#define HISTORICO_H

/*
 * historico.c — todas as partidas jogadas, guardadas em arquivo binário.
 *
 * O arquivo assets/dados/historico.bin é só uma sequência de structs
 * RegistroPartida, uma atrás da outra. Cada partida nova é gravada no FIM
 * do arquivo (modo "ab"), sem reescrever as antigas.
 */

#include "jogo.h"
#include <stddef.h>

#define PASTA_DADOS       "assets/dados"
#define ARQUIVO_HISTORICO "assets/dados/historico.bin"

/* Cria a pasta assets/dados se ela não existir. */
void garantir_pasta_dados(void);

void historico_iniciar(Historico *h);
void historico_liberar(Historico *h);

/*
 * Lê o arquivo inteiro para o vetor e deixa o vetor ordenado por pontos.
 * Arquivo inexistente não é erro (primeira vez jogando).
 * Se o arquivo estiver cortado ou com registros estranhos, lê o que der,
 * descarta o resto e explica em "aviso". Retorna quantos registros leu.
 */
int historico_carregar(Historico *h, char *aviso, size_t tam_aviso);

/* Grava um registro no fim do arquivo. Retorna 1 se deu certo. */
int historico_gravar(const RegistroPartida *r);

/*
 * Insere o registro no vetor, já na posição certa pela pontuação
 * (o mesmo passo que o Insertion Sort faz para cada elemento).
 * Retorna a posição (0 = primeiro lugar) ou -1 se faltar memória.
 */
int historico_inserir_ordenado(Historico *h, RegistroPartida r);

int calcular_pontos(const RegistroPartida *r, int bonus);

/* Percorre o histórico inteiro somando vitórias, derrotas, recordes... */
Estatisticas calcular_estatisticas(const Historico *h);

#endif
