#ifndef ANDAR_H
#define ANDAR_H

/*
 * andar.c — lê os andares da torre de arquivos de texto.
 *
 * Formato de assets/andares/andarN.txt:
 *
 *     // linhas começando com // são comentários (só antes do mapa)
 *     nome: O Salão das Colunas
 *     ondas: 2
 *     mapa:
 *     ####################      <- 13 linhas de 20 caracteres
 *     #........S.........#         '#' parede   '.' chão
 *     ...                          'M' onde o mago começa (exatamente 1)
 *     ####################         'S' onde fica a escada (exatamente 1)
 */

#include "jogo.h"
#include <stddef.h>

#define PASTA_ANDARES "assets/andares"
#define TOTAL_ANDARES 5

/*
 * Lê e valida um arquivo de andar. Retorna 1 se deu certo.
 * Se der errado, retorna 0 e escreve em "erro" o motivo e a linha do problema.
 */
int carregar_andar_de_arquivo(const char *caminho, Andar *a, char *erro, size_t tam_erro);

/*
 * Carrega o andar "numero". Se o arquivo faltar ou estiver inválido, usa o
 * andar padrão (que fica dentro do código) e escreve o motivo em "aviso".
 * Se tudo der certo, "aviso" fica vazio.
 */
void carregar_andar(int numero, Andar *a, char *aviso, size_t tam_aviso);

/*
 * Conta quantos tiles de chão dá para alcançar andando a partir de (x, y).
 * É recursiva: marca o tile atual e chama a si mesma para os 4 vizinhos.
 * "visitado" precisa começar todo zerado.
 */
int contar_alcancaveis(const Mapa *m, int x, int y, int visitado[MAPA_ALTURA][MAPA_LARGURA]);

/* Troca letras acentuadas (UTF-8) pela versão sem acento: "Salão" -> "Salao". */
void remover_acentos(const char *origem, char *destino, size_t tam_destino);

#endif
