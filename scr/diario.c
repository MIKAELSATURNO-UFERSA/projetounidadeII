#include "diario.h"
#include "historico.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TAMANHO_MAXIMO_DIARIO (256L * 1024L)   /* 256 KB */

/* Abre o diário para acrescentar no fim. Devolve NULL se não conseguir. */
static FILE *abrir_para_acrescentar(void) {
    garantir_pasta_dados();
    return fopen(ARQUIVO_DIARIO, "a");
}

/* Se o diário ficar muito grande, ele vira diario_antigo.txt e começa outro. */
static void girar_se_grande(void) {
    FILE *fp = fopen(ARQUIVO_DIARIO, "rb");
    if (fp == NULL) return;
    fseek(fp, 0, SEEK_END);
    long tamanho = ftell(fp);
    fclose(fp);

    if (tamanho > TAMANHO_MAXIMO_DIARIO) {
        remove(ARQUIVO_DIARIO_ANTIGO);
        rename(ARQUIVO_DIARIO, ARQUIVO_DIARIO_ANTIGO);
    }
}

void diario_comecar_partida(const char *jogador) {
    girar_se_grande();
    FILE *fp = abrir_para_acrescentar();
    if (fp == NULL) return;      /* sem diário a partida continua normalmente */

    char data[32] = "?";
    time_t agora = time(NULL);
    struct tm *t = localtime(&agora);
    if (t != NULL) strftime(data, sizeof(data), "%d/%m/%Y %H:%M", t);

    fprintf(fp, "\n=== %s | %s ===\n", jogador, data);
    fclose(fp);
}

void diario_escrever(float tempo_partida, int andar, const char *texto) {
    FILE *fp = abrir_para_acrescentar();
    if (fp == NULL) return;

    int s = (int)tempo_partida;
    fprintf(fp, "[%02d:%02d] Andar %d | %s\n", s / 60, s % 60, andar, texto);
    fclose(fp);   /* fechar a cada linha: se o jogo fechar de repente, nada se perde */
}

void diario_terminar_partida(int venceu, int andar, int pontos) {
    FILE *fp = abrir_para_acrescentar();
    if (fp == NULL) return;

    if (venceu) fprintf(fp, "--- VITORIA! Chegou ao topo | %d pontos ---\n", pontos);
    else        fprintf(fp, "--- Caiu no andar %d | %d pontos ---\n", andar, pontos);
    fclose(fp);
}

int diario_ultimas_linhas(char linhas[][TAM_LINHA_DIARIO], int max) {
    if (max <= 0) return 0;
    if (max > MAX_LINHAS_RESUMO) max = MAX_LINHAS_RESUMO;

    FILE *fp = fopen(ARQUIVO_DIARIO, "r");
    if (fp == NULL) return 0;

    /*
     * Fila circular: "fila" guarda no máximo "max" linhas. Quando enche, a
     * linha nova entra no lugar da mais antiga (posição "inicio"), e o
     * início anda uma casa. No fim, sobram exatamente as últimas linhas.
     */
    char fila[MAX_LINHAS_RESUMO][TAM_LINHA_DIARIO];
    int  inicio = 0, quantidade = 0;
    char linha[TAM_LINHA_DIARIO];

    while (fgets(linha, sizeof(linha), fp) != NULL) {     /* até o EOF */
        size_t n = strlen(linha);
        if (n > 0 && linha[n - 1] != '\n') {
            /* linha maior que o buffer: descarta o resto dela */
            int c;
            while ((c = fgetc(fp)) != '\n' && c != EOF) { }
        }
        while (n > 0 && (linha[n - 1] == '\n' || linha[n - 1] == '\r')) linha[--n] = '\0';
        if (n == 0) continue;                               /* ignora linhas vazias */

        int pos;
        if (quantidade < max) {
            pos = (inicio + quantidade) % max;
            quantidade++;
        } else {
            pos = inicio;                                   /* sobrescreve a mais antiga */
            inicio = (inicio + 1) % max;
        }
        memcpy(fila[pos], linha, n + 1);
    }
    fclose(fp);

    for (int i = 0; i < quantidade; i++) {                  /* copia na ordem certa */
        memcpy(linhas[i], fila[(inicio + i) % max], TAM_LINHA_DIARIO);
    }
    return quantidade;
}

int ler_ultimo_jogador(char *nome, size_t tam) {
    if (nome == NULL || tam == 0) return 0;
    nome[0] = '\0';

    FILE *fp = fopen(ARQUIVO_JOGADOR, "r");
    if (fp == NULL) return 0;

    char linha[64];
    if (fgets(linha, sizeof(linha), fp) != NULL) {
        size_t n = strlen(linha);
        while (n > 0 && (linha[n - 1] == '\n' || linha[n - 1] == '\r' || linha[n - 1] == ' ')) linha[--n] = '\0';
        /* aceita só caracteres comuns, para não mostrar lixo na tela */
        size_t j = 0;
        for (size_t i = 0; i < n && j + 1 < tam; i++) {
            if (linha[i] >= 32 && linha[i] <= 126) nome[j++] = linha[i];
        }
        nome[j] = '\0';
    }
    fclose(fp);
    return nome[0] != '\0';
}

void salvar_ultimo_jogador(const char *nome) {
    garantir_pasta_dados();
    FILE *fp = fopen(ARQUIVO_JOGADOR, "w");
    if (fp == NULL) return;
    fprintf(fp, "%s\n", nome);
    fclose(fp);
}
