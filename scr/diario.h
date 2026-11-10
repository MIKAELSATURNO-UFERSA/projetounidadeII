#ifndef DIARIO_H
#define DIARIO_H

/*
 * diario.c — o diário de combate, um arquivo de TEXTO que qualquer pessoa
 * pode abrir no Bloco de Notas (assets/dados/diario.txt).
 *
 * É um histórico incremental: cada acontecimento vira uma linha nova no
 * fim do arquivo (modo "a"), e as partidas antigas continuam lá em cima.
 */

#include <stddef.h>

#define ARQUIVO_DIARIO         "assets/dados/diario.txt"
#define ARQUIVO_DIARIO_ANTIGO  "assets/dados/diario_antigo.txt"
#define ARQUIVO_JOGADOR        "assets/dados/jogador.txt"

#define MAX_LINHAS_RESUMO  6
#define TAM_LINHA_DIARIO   128

void diario_comecar_partida(const char *jogador);
void diario_escrever(float tempo_partida, int andar, const char *texto);
void diario_terminar_partida(int venceu, int andar, int pontos);

/*
 * Lê o diário até o fim do arquivo (EOF) e guarda só as últimas "max"
 * linhas, usando uma fila circular. Retorna quantas linhas guardou.
 */
int diario_ultimas_linhas(char linhas[][TAM_LINHA_DIARIO], int max);

/* Nome usado na última partida (para já aparecer escrito na próxima). */
int  ler_ultimo_jogador(char *nome, size_t tam);
void salvar_ultimo_jogador(const char *nome);

#endif
