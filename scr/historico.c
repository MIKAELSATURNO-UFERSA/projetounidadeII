#include "historico.h"
#include "andar.h"
#include "ordenacao.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Criar pasta tem um nome diferente no Windows e no Linux. */
#ifdef _WIN32
    #include <direct.h>
    #define CRIAR_PASTA(caminho) _mkdir(caminho)
#else
    #include <sys/stat.h>
    #define CRIAR_PASTA(caminho) mkdir(caminho, 0755)
#endif

void garantir_pasta_dados(void) {
    /* Se a pasta já existir, CRIAR_PASTA só falha em silêncio — sem problema. */
    CRIAR_PASTA("assets");
    CRIAR_PASTA(PASTA_DADOS);
}

/* ======================= vetor dinâmico ======================= */

void historico_iniciar(Historico *h) {
    h->itens = NULL;
    h->total = 0;
    h->capacidade = 0;
}

void historico_liberar(Historico *h) {
    free(h->itens);
    historico_iniciar(h);
}

/* Garante espaço para pelo menos "minimo" registros (dobra a capacidade). */
static int reservar(Historico *h, int minimo) {
    if (h->capacidade >= minimo) return 1;

    int nova = (h->capacidade > 0) ? h->capacidade : 8;
    while (nova < minimo) nova *= 2;

    RegistroPartida *temp = (RegistroPartida *) realloc(h->itens, nova * sizeof(RegistroPartida));
    if (temp == NULL) {
        TraceLog(LOG_ERROR, "Sem memoria para o historico.");
        return 0;
    }
    h->itens = temp;
    h->capacidade = nova;
    return 1;
}

/* ======================= arquivo ======================= */

/* Um registro lido do arquivo faz sentido? (protege contra arquivo corrompido) */
static int registro_valido(RegistroPartida *r) {
    r->nome[TAM_NOME - 1] = '\0';                 /* garante o fim da string */
    if (r->nome[0] == '\0') return 0;
    if (r->venceu != 0 && r->venceu != 1) return 0;
    if (r->andar < 1 || r->andar > TOTAL_ANDARES) return 0;
    if (r->tempo < 0 || r->ecos_derrotados < 0 || r->pontos < 0) return 0;
    return 1;
}

int historico_carregar(Historico *h, char *aviso, size_t tam_aviso) {
    if (aviso != NULL && tam_aviso > 0) aviso[0] = '\0';
    h->total = 0;

    FILE *fp = fopen(ARQUIVO_HISTORICO, "rb");
    if (fp == NULL) return 0;                     /* ainda não existe: histórico vazio */

    /* Tamanho do arquivo / tamanho de um registro = quantos registros existem. */
    fseek(fp, 0, SEEK_END);
    long tamanho = ftell(fp);
    rewind(fp);

    if (tamanho <= 0) {
        fclose(fp);
        return 0;
    }

    long quantidade = tamanho / (long)sizeof(RegistroPartida);
    long sobra      = tamanho % (long)sizeof(RegistroPartida);

    if (quantidade == 0 || !reservar(h, (int)quantidade)) {
        fclose(fp);
        if (aviso != NULL) snprintf(aviso, tam_aviso, "historico.bin incompleto ou grande demais; comecando vazio.");
        return 0;
    }

    /* Um fread só lê o vetor inteiro de registros. */
    size_t lidos = fread(h->itens, sizeof(RegistroPartida), (size_t)quantidade, fp);
    fclose(fp);

    /* Descarta registros com valores impossíveis (arquivo corrompido). */
    int validos = 0, descartados = 0;
    for (size_t i = 0; i < lidos; i++) {
        if (registro_valido(&h->itens[i])) h->itens[validos++] = h->itens[i];
        else descartados++;
    }
    h->total = validos;

    if (aviso != NULL && (sobra != 0 || lidos != (size_t)quantidade || descartados > 0)) {
        snprintf(aviso, tam_aviso,
                 "historico.bin com problema: %d registro(s) descartado(s), %ld byte(s) sobrando.",
                 descartados + (int)((size_t)quantidade - lidos), sobra);
        TraceLog(LOG_WARNING, "%s", aviso);
    }

    /* O vetor fica ordenado por pontos (é a ordem do ranking principal). */
    merge_sort_registros(h->itens, h->total, CRITERIO_PONTOS, NULL);
    return h->total;
}

int historico_gravar(const RegistroPartida *r) {
    garantir_pasta_dados();

    FILE *fp = fopen(ARQUIVO_HISTORICO, "ab");   /* "a" = acrescenta no fim */
    if (fp == NULL) {
        TraceLog(LOG_WARNING, "Nao foi possivel abrir %s para gravar.", ARQUIVO_HISTORICO);
        return 0;
    }
    size_t escritos = fwrite(r, sizeof(RegistroPartida), 1, fp);
    fclose(fp);
    return escritos == 1;
}

/* ======================= inserção e cálculos ======================= */

int historico_inserir_ordenado(Historico *h, RegistroPartida r) {
    if (!reservar(h, h->total + 1)) return -1;

    /* Começa no fim e vai empurrando para a direita quem tem que ficar depois. */
    int i = h->total;
    while (i > 0 && comparar_registros(&r, &h->itens[i - 1], CRITERIO_PONTOS) < 0) {
        h->itens[i] = h->itens[i - 1];
        i--;
    }
    h->itens[i] = r;
    h->total++;
    return i;
}

int calcular_pontos(const RegistroPartida *r, int bonus) {
    int pontos = r->ecos_derrotados * 10 + (r->andar - 1) * 250 + bonus;
    if (r->venceu) {
        pontos += 1500;
        int rapidez = 900 - r->tempo;              /* bônus por vencer rápido */
        if (rapidez > 0) pontos += rapidez;
    }
    return pontos;
}

Estatisticas calcular_estatisticas(const Historico *h) {
    Estatisticas e;
    memset(&e, 0, sizeof(e));

    for (int i = 0; i < h->total; i++) {         /* um passeio pelo vetor: O(n) */
        const RegistroPartida *r = &h->itens[i];
        e.partidas++;
        if (r->venceu) e.vitorias++;
        else           e.derrotas++;
        if (r->andar  > e.melhor_andar)  e.melhor_andar  = r->andar;
        if (r->pontos > e.melhor_pontos) e.melhor_pontos = r->pontos;
    }
    return e;
}
