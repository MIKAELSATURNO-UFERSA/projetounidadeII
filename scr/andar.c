#include "andar.h"
#include "mapa.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define TAM_LINHA            256
#define MAX_ONDAS_POR_ANDAR    4

/*
 * Andar de reserva: usado quando o arquivo do andar não existe ou tem erro.
 * Assim o jogo nunca fecha por causa de um arquivo quebrado.
 */
static const char *NOME_PADRAO = "A Entrada";
static const char *MAPA_PADRAO[MAPA_ALTURA] = {
    "####################",
    "#........S.........#",
    "#..##..........##..#",
    "#..#............#..#",
    "#......#....#......#",
    "#......#....#......#",
    "#.........M........#",
    "#......#....#......#",
    "#......#....#......#",
    "#..#............#..#",
    "#..##..........##..#",
    "#..................#",
    "####################",
};

/* =================== transformação de texto =================== */

/*
 * A fonte padrão da raylib não garante letras acentuadas, então o nome do
 * andar é transformado: cada letra acentuada (que em UTF-8 ocupa 2 bytes)
 * vira a letra simples (1 byte).
 */
typedef struct {
    const char *com;
    char        sem;
} TrocaAcento;

static const TrocaAcento TROCAS[] = {
    { "á", 'a' }, { "à", 'a' }, { "â", 'a' }, { "ã", 'a' }, { "ä", 'a' },
    { "é", 'e' }, { "è", 'e' }, { "ê", 'e' },
    { "í", 'i' }, { "ì", 'i' }, { "î", 'i' },
    { "ó", 'o' }, { "ò", 'o' }, { "ô", 'o' }, { "õ", 'o' }, { "ö", 'o' },
    { "ú", 'u' }, { "ù", 'u' }, { "û", 'u' }, { "ü", 'u' },
    { "ç", 'c' }, { "ñ", 'n' },
    { "Á", 'A' }, { "À", 'A' }, { "Â", 'A' }, { "Ã", 'A' },
    { "É", 'E' }, { "Ê", 'E' }, { "Í", 'I' },
    { "Ó", 'O' }, { "Ô", 'O' }, { "Õ", 'O' },
    { "Ú", 'U' }, { "Ç", 'C' },
};
#define TOTAL_TROCAS ((int)(sizeof(TROCAS) / sizeof(TROCAS[0])))

void remover_acentos(const char *origem, char *destino, size_t tam_destino) {
    if (destino == NULL || tam_destino == 0) return;
    size_t i = 0, j = 0;

    while (origem[i] != '\0' && j + 1 < tam_destino) {
        unsigned char c = (unsigned char)origem[i];
        if (c < 0x80) {                 /* caractere comum (ASCII): copia */
            destino[j++] = origem[i++];
            continue;
        }

        int trocou = 0;
        for (int k = 0; k < TOTAL_TROCAS; k++) {      /* busca linear na tabela */
            size_t n = strlen(TROCAS[k].com);
            if (strncmp(&origem[i], TROCAS[k].com, n) == 0) {
                destino[j++] = TROCAS[k].sem;
                i += n;
                trocou = 1;
                break;
            }
        }
        if (!trocou) {                  /* símbolo desconhecido: vira '?' */
            destino[j++] = '?';
            i++;
            while (((unsigned char)origem[i] & 0xC0) == 0x80) i++;   /* resto do caractere */
        }
    }
    destino[j] = '\0';
}

/* =================== ajudantes de leitura =================== */

/* Escreve a mensagem de erro (como printf) e retorna 0, para usar em "return falhar(...)". */
static int falhar(char *erro, size_t tam_erro, const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    vsnprintf(erro, tam_erro, formato, args);
    va_end(args);
    return 0;
}

/* Tira o '\n' do fgets, o '\r' de arquivos do Windows e espaços sobrando no fim. */
static void limpar_fim(char *linha) {
    size_t n = strlen(linha);
    while (n > 0 && (linha[n - 1] == '\n' || linha[n - 1] == '\r' ||
                     linha[n - 1] == ' '  || linha[n - 1] == '\t')) {
        linha[--n] = '\0';
    }
}

/* O Bloco de Notas às vezes grava 3 bytes invisíveis (BOM) no começo do arquivo. */
static void remover_bom(char *linha) {
    if ((unsigned char)linha[0] == 0xEF && (unsigned char)linha[1] == 0xBB &&
        (unsigned char)linha[2] == 0xBF) {
        memmove(linha, linha + 3, strlen(linha + 3) + 1);
    }
}

static const char *pular_espacos(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

/* =================== flood fill recursivo =================== */

int contar_alcancaveis(const Mapa *m, int x, int y, int visitado[MAPA_ALTURA][MAPA_LARGURA]) {
    /* Caso base: parede (ou fora do mapa) ou tile já visitado não conta nada. */
    if (eh_parede(m, x, y)) return 0;
    if (visitado[y][x])     return 0;

    /* Marca ANTES de chamar os vizinhos; senão um chama o outro para sempre. */
    visitado[y][x] = 1;

    /* Passo recursivo: este tile + tudo o que se alcança por cada um dos 4 vizinhos. */
    return 1 + contar_alcancaveis(m, x + 1, y, visitado)
             + contar_alcancaveis(m, x - 1, y, visitado)
             + contar_alcancaveis(m, x, y + 1, visitado)
             + contar_alcancaveis(m, x, y - 1, visitado);
}

/* =================== validação e conversão =================== */

/*
 * Transforma as 13 linhas de texto no Mapa e confere as regras.
 * linha_no_arquivo[y] diz em que linha do .txt estava a linha y do mapa,
 * para a mensagem de erro apontar o lugar certo.
 */
static int converter_grade(char grade[MAPA_ALTURA][MAPA_LARGURA + 1],
                           const int linha_no_arquivo[MAPA_ALTURA],
                           Andar *a, char *erro, size_t tam_erro) {
    int qtd_m = 0, qtd_s = 0, total_chao = 0;

    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            char c = grade[y][x];
            int  num = linha_no_arquivo[y];

            switch (c) {
            case '#': a->mapa.tiles[y][x] = TILE_PAREDE; break;
            case '.': a->mapa.tiles[y][x] = TILE_CHAO;   break;
            case 'M': a->mapa.tiles[y][x] = TILE_CHAO; a->inicio_x = x; a->inicio_y = y; qtd_m++; break;
            case 'S': a->mapa.tiles[y][x] = TILE_CHAO; a->escada_x = x; a->escada_y = y; qtd_s++; break;
            default:
                return falhar(erro, tam_erro, "linha %d, coluna %d: o caractere '%c' nao existe no mapa",
                              num, x + 1, c);
            }

            int na_borda = (x == 0 || y == 0 || x == MAPA_LARGURA - 1 || y == MAPA_ALTURA - 1);
            if (na_borda && c != '#') {
                return falhar(erro, tam_erro, "linha %d, coluna %d: a borda do mapa precisa ser parede '#'",
                              num, x + 1);
            }
            if (c != '#') total_chao++;
        }
    }

    if (qtd_m != 1) return falhar(erro, tam_erro, "o mapa precisa de exatamente 1 'M' (inicio), tem %d", qtd_m);
    if (qtd_s != 1) return falhar(erro, tam_erro, "o mapa precisa de exatamente 1 'S' (escada), tem %d", qtd_s);

    /* Todo chão precisa ser alcançável a partir do 'M' (senão um eco pode nascer preso). */
    int visitado[MAPA_ALTURA][MAPA_LARGURA];
    memset(visitado, 0, sizeof(visitado));
    int alcancaveis = contar_alcancaveis(&a->mapa, a->inicio_x, a->inicio_y, visitado);

    if (alcancaveis < total_chao) {
        for (int y = 0; y < MAPA_ALTURA; y++) {
            for (int x = 0; x < MAPA_LARGURA; x++) {
                if (a->mapa.tiles[y][x] == TILE_CHAO && !visitado[y][x]) {
                    return falhar(erro, tam_erro,
                                  "linha %d, coluna %d: chao isolado, o mago nao consegue chegar ali",
                                  linha_no_arquivo[y], x + 1);
                }
            }
        }
    }
    return 1;
}

/* Lê o arquivo linha a linha e separa: nome, ondas e as linhas do mapa. */
static int ler_linhas(FILE *fp, Andar *novo, char grade[MAPA_ALTURA][MAPA_LARGURA + 1],
                      int linha_no_arquivo[MAPA_ALTURA], char *erro, size_t tam_erro) {
    char linha[TAM_LINHA];
    int  num = 0, linhas_mapa = 0;
    int  tem_nome = 0, tem_ondas = 0, lendo_mapa = 0;

    while (fgets(linha, sizeof(linha), fp) != NULL) {     /* para no fim do arquivo (EOF) */
        num++;
        if (strchr(linha, '\n') == NULL && !feof(fp)) {
            return falhar(erro, tam_erro, "linha %d: linha comprida demais", num);
        }
        if (num == 1) remover_bom(linha);
        limpar_fim(linha);

        if (!lendo_mapa) {
            const char *t = pular_espacos(linha);
            if (*t == '\0' || strncmp(t, "//", 2) == 0) continue;   /* vazia ou comentário */

            if (strncmp(t, "nome:", 5) == 0) {
                remover_acentos(pular_espacos(t + 5), novo->nome, sizeof(novo->nome));
                if (novo->nome[0] == '\0') return falhar(erro, tam_erro, "linha %d: o nome esta vazio", num);
                tem_nome = 1;

            } else if (strncmp(t, "ondas:", 6) == 0) {
                int  n;
                char sobra;
                if (sscanf(t + 6, "%d %c", &n, &sobra) != 1 || n < 1 || n > MAX_ONDAS_POR_ANDAR) {
                    return falhar(erro, tam_erro, "linha %d: 'ondas' precisa ser um numero de 1 a %d",
                                  num, MAX_ONDAS_POR_ANDAR);
                }
                novo->ondas = n;
                tem_ondas = 1;

            } else if (strcmp(t, "mapa:") == 0) {
                lendo_mapa = 1;

            } else {
                return falhar(erro, tam_erro, "linha %d: nao entendi \"%.20s\" (esperava nome:, ondas: ou mapa:)",
                              num, t);
            }

        } else {
            if (linha[0] == '\0') {
                if (linhas_mapa == MAPA_ALTURA) continue;   /* linhas vazias no fim: tudo bem */
                return falhar(erro, tam_erro, "linha %d: linha vazia no meio do mapa", num);
            }
            if (linhas_mapa == MAPA_ALTURA) {
                return falhar(erro, tam_erro, "linha %d: o mapa passou de %d linhas", num, MAPA_ALTURA);
            }
            int tam = (int)strlen(linha);
            if (tam != MAPA_LARGURA) {
                return falhar(erro, tam_erro, "linha %d: o mapa precisa de %d caracteres por linha, essa tem %d",
                              num, MAPA_LARGURA, tam);
            }
            memcpy(grade[linhas_mapa], linha, MAPA_LARGURA + 1);
            linha_no_arquivo[linhas_mapa] = num;
            linhas_mapa++;
        }
    }

    if (!tem_nome)   return falhar(erro, tam_erro, "faltou a linha 'nome:'");
    if (!tem_ondas)  return falhar(erro, tam_erro, "faltou a linha 'ondas:'");
    if (!lendo_mapa) return falhar(erro, tam_erro, "faltou a linha 'mapa:'");
    if (linhas_mapa < MAPA_ALTURA) {
        return falhar(erro, tam_erro, "o mapa tem so %d linhas, precisa de %d", linhas_mapa, MAPA_ALTURA);
    }
    return 1;
}

int carregar_andar_de_arquivo(const char *caminho, Andar *a, char *erro, size_t tam_erro) {
    if (caminho == NULL || a == NULL || erro == NULL) return 0;

    FILE *fp = fopen(caminho, "r");
    if (fp == NULL) return falhar(erro, tam_erro, "arquivo nao encontrado");

    Andar novo;   /* só é copiado para *a se o arquivo inteiro for válido */
    memset(&novo, 0, sizeof(novo));
    char grade[MAPA_ALTURA][MAPA_LARGURA + 1];
    int  linha_no_arquivo[MAPA_ALTURA];

    int ok = ler_linhas(fp, &novo, grade, linha_no_arquivo, erro, tam_erro);
    fclose(fp);

    if (!ok) return 0;
    if (!converter_grade(grade, linha_no_arquivo, &novo, erro, tam_erro)) return 0;

    *a = novo;
    return 1;
}

/* =================== andar de reserva =================== */

static void andar_padrao(Andar *a) {
    memset(a, 0, sizeof(*a));
    strncpy(a->nome, NOME_PADRAO, sizeof(a->nome) - 1);
    a->ondas = 2;

    char grade[MAPA_ALTURA][MAPA_LARGURA + 1];
    int  linhas[MAPA_ALTURA];
    for (int y = 0; y < MAPA_ALTURA; y++) {
        memcpy(grade[y], MAPA_PADRAO[y], MAPA_LARGURA + 1);
        linhas[y] = y + 1;
    }

    char erro[160];
    if (!converter_grade(grade, linhas, a, erro, sizeof(erro))) {
        /* Só aconteceria se alguém editasse o MAPA_PADRAO errado: sala vazia. */
        TraceLog(LOG_ERROR, "Andar padrao invalido (%s). Usando sala vazia.", erro);
        for (int y = 0; y < MAPA_ALTURA; y++)
            for (int x = 0; x < MAPA_LARGURA; x++)
                a->mapa.tiles[y][x] = (x == 0 || y == 0 || x == MAPA_LARGURA - 1 ||
                                       y == MAPA_ALTURA - 1) ? TILE_PAREDE : TILE_CHAO;
        a->inicio_x = MAPA_LARGURA / 2; a->inicio_y = MAPA_ALTURA / 2;
        a->escada_x = MAPA_LARGURA / 2; a->escada_y = 1;
    }
}

void carregar_andar(int numero, Andar *a, char *aviso, size_t tam_aviso) {
    char caminho[64];
    char erro[160];
    snprintf(caminho, sizeof(caminho), "%s/andar%d.txt", PASTA_ANDARES, numero);
    if (aviso != NULL && tam_aviso > 0) aviso[0] = '\0';

    if (carregar_andar_de_arquivo(caminho, a, erro, sizeof(erro))) return;

    if (aviso != NULL && tam_aviso > 0) {
        snprintf(aviso, tam_aviso, "andar%d.txt: %s. Usando o mapa padrao.", numero, erro);
        TraceLog(LOG_WARNING, "%s", aviso);
    }
    andar_padrao(a);
}
