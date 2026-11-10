#include "ui.h"
#include "projetil.h"
#include "melhoria.h"
#include "ordenacao.h"
#include "diario.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ---------- ajudantes ---------- */

static void texto_centro(const char *texto, int y, int tam, Color cor) {
    DrawText(texto, LARGURA_TELA / 2 - MeasureText(texto, tam) / 2, y, tam, cor);
}

static void formatar_tempo(char *buf, size_t tam, float segundos) {
    int s = (int)segundos;
    snprintf(buf, tam, "%02d:%02d", s / 60, s % 60);
}

/* Copia o texto cortando o final (com "...") até caber na largura dada. */
static void texto_que_cabe(const char *texto, char *saida, size_t tam_saida, int largura, int tam_fonte) {
    snprintf(saida, tam_saida, "%s", texto);
    size_t n = strlen(saida);
    while (n > 3 && MeasureText(saida, tam_fonte) > largura) {
        n--;
        saida[n] = '\0';
        saida[n - 1] = '.';
        saida[n - 2] = '.';
        saida[n - 3] = '.';
    }
}

static void formatar_data(char *buf, size_t tam, long long data) {
    time_t quando = (time_t)data;
    struct tm *t = localtime(&quando);
    if (t == NULL || strftime(buf, tam, "%d/%m %H:%M", t) == 0) snprintf(buf, tam, "?");
}

/* Barrinha de recarga: cheia = pronto para usar. */
static void barra_recarga(int x, int y, const char *nome, float pct) {
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    int pronto = (pct >= 1.0f);

    DrawText(nome, x, y, 12, pronto ? (Color){ 200, 230, 255, 255 }
                                    : (Color){ 120, 120, 150, 255 });
    DrawRectangle(x, y + 14, 110, 6, (Color){ 40, 40, 60, 255 });
    DrawRectangle(x, y + 14, (int)(110 * pct), 6,
                  pronto ? (Color){ 120, 200, 255, 255 } : (Color){ 80, 110, 160, 255 });
}

/* ---------- durante o jogo ---------- */

void desenhar_hud(const Partida *p) {
    const Personagem *mago = &p->mago;
    const Jogo *j = &p->jogo;
    char buf[80];

    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_HUD, (Color){ 20, 20, 40, 255 });

    DrawText("O ULTIMO ANDAR", 20, 10, 24, (Color){ 120, 200, 255, 255 });
    snprintf(buf, sizeof(buf), "Andar %d/%d: %s", j->andar, j->andar_max, p->andar.nome);
    DrawText(buf, 20, 40, 14, (Color){ 180, 180, 200, 255 });

    snprintf(buf, sizeof(buf), "Onda: %d/%d", j->onda > 0 ? j->onda : 1, j->ondas_no_andar);
    DrawText(buf, 290, 10, 20, (Color){ 200, 180, 255, 255 });
    if (j->escada_aberta) {
        DrawText("ESCADA ABERTA!", 290, 38, 16, (Color){ 255, 220, 120, 255 });
    } else {
        snprintf(buf, sizeof(buf), "Ecos: %d", p->ecos.total);
        DrawText(buf, 290, 38, 18, (Color){ 220, 140, 255, 255 });
    }

    char tempo[16];
    formatar_tempo(tempo, sizeof(tempo), j->tempo_partida);
    snprintf(buf, sizeof(buf), "Tempo: %s", tempo);
    DrawText(buf, 430, 10, 20, RAYWHITE);
    snprintf(buf, sizeof(buf), "Derrotados: %d", j->ecos_derrotados);
    DrawText(buf, 430, 38, 18, (Color){ 160, 230, 200, 255 });

    snprintf(buf, sizeof(buf), "VIDA: %d/%d", mago->vida, mago->vida_max);
    DrawText(buf, 600, 8, 20, (Color){ 255, 150, 150, 255 });
    int bx = 600, by = 34, bl = 200, ba = 16;
    DrawRectangle(bx, by, bl, ba, (Color){ 60, 30, 30, 255 });
    DrawRectangle(bx, by, (int)(bl * (float)mago->vida / mago->vida_max), ba,
                  (Color){ 220, 60, 60, 255 });
    DrawRectangleLines(bx, by, bl, ba, (Color){ 120, 120, 140, 255 });

    barra_recarga(825, 8, "ESFERA",
                  1.0f - mago->recarga_esfera / mago->tempo_entre_esferas);
    barra_recarga(825, 36, "ESQUIVA",
                  1.0f - mago->recarga_esquiva / mago->tempo_recarga_esquiva);
}

void desenhar_rodape(const Partida *p) {
    int y = ALTURA_HUD + ALTURA_SALA;
    DrawRectangle(0, y, LARGURA_TELA, ALTURA_RODAPE, (Color){ 14, 14, 26, 255 });

    /* mensagem do último acontecimento, que vai apagando */
    float alfa = (p->jogo.tempo_mensagem > 1.0f) ? 1.0f : 0.35f + 0.65f * p->jogo.tempo_mensagem;
    DrawText(p->jogo.mensagem, 20, y + 4, 16, Fade((Color){ 220, 220, 180, 255 }, alfa));

    DrawText("WASD: mover   MOUSE: mirar   CLIQUE: esfera   ESPACO: esquiva   "
             "P: pausa   F1: debug   ESC: sair",
             20, y + 23, 13, (Color){ 130, 130, 150, 255 });
}

void desenhar_mira(void) {
    Vector2 m = GetMousePosition();
    if (m.y < ALTURA_HUD || m.y > ALTURA_HUD + ALTURA_SALA) return;
    DrawCircleLines((int)m.x, (int)m.y, 10, (Color){ 160, 210, 255, 200 });
    DrawCircleV(m, 2.0f, (Color){ 220, 240, 255, 255 });
}

void desenhar_aviso_intervalo(const Jogo *j) {
    int meio = ALTURA_HUD + ALTURA_SALA / 2;
    DrawRectangle(0, meio - 60, LARGURA_TELA, 110, Fade(BLACK, 0.55f));

    char buf[64];
    snprintf(buf, sizeof(buf), "ONDA %d LIMPA!", j->onda);
    texto_centro(buf, meio - 48, 40, (Color){ 120, 255, 180, 255 });

    snprintf(buf, sizeof(buf), "Proxima onda em %d...", (int)ceilf(j->intervalo));
    texto_centro(buf, meio + 6, 24, (Color){ 220, 180, 255, 255 });
}

void desenhar_titulo_andar(const Partida *p) {
    float t = p->jogo.tempo_titulo;
    if (t <= 0.0f) return;
    float alfa = (t > 0.6f) ? 1.0f : t / 0.6f;     /* some aos poucos no final */

    int meio = ALTURA_HUD + ALTURA_SALA / 2;
    DrawRectangle(0, meio - 70, LARGURA_TELA, 130, Fade(BLACK, 0.6f * alfa));

    char buf[64];
    snprintf(buf, sizeof(buf), "ANDAR %d DE %d", p->jogo.andar, p->jogo.andar_max);
    texto_centro(buf, meio - 58, 22, Fade((Color){ 200, 180, 255, 255 }, alfa));
    texto_centro(p->andar.nome, meio - 26, 44, Fade((Color){ 255, 230, 160, 255 }, alfa));
}

/* Se o .txt do andar deu erro, mostra o motivo em vermelho no topo da sala. */
void desenhar_aviso_arquivo(const Partida *p) {
    if (p->aviso_andar[0] == '\0') return;
    int largura = MeasureText(p->aviso_andar, 14);
    DrawRectangle(6, ALTURA_HUD + 6, largura + 16, 22, Fade(BLACK, 0.75f));
    DrawText(p->aviso_andar, 14, ALTURA_HUD + 10, 14, (Color){ 255, 110, 110, 255 });
}

void desenhar_debug(const Partida *p) {
    int n = p->ecos.total;
    char buf[96];
    int x = LARGURA_TELA - 330, y = ALTURA_HUD + 8;

    DrawRectangle(x - 8, y - 6, 324, 144, Fade(BLACK, 0.7f));
    DrawText("DEBUG (F1)", x, y, 16, YELLOW);

    snprintf(buf, sizeof(buf), "FPS: %d   Ecos: %d   Projeteis: %d",
             GetFPS(), n, contar_projeteis_ativos(&p->projeteis));
    DrawText(buf, x, y + 22, 14, RAYWHITE);

    snprintf(buf, sizeof(buf), "Insertion Sort: %d comparacoes",
             p->desempenho.comparacoes_ordenacao);
    DrawText(buf, x, y + 42, 14, (Color){ 140, 255, 180, 255 });
    snprintf(buf, sizeof(buf), "   (n-1 = %d, pior caso n(n-1)/2 = %d)",
             n > 0 ? n - 1 : 0, n * (n - 1) / 2);
    DrawText(buf, x, y + 58, 12, (Color){ 140, 180, 160, 255 });

    snprintf(buf, sizeof(buf), "Separacao: %ld testes",
             p->desempenho.testes_separacao);
    DrawText(buf, x, y + 78, 14, (Color){ 255, 180, 140, 255 });
    snprintf(buf, sizeof(buf), "   (n(n-1) = %d)", n * (n - 1));
    DrawText(buf, x, y + 94, 12, (Color){ 180, 150, 130, 255 });

    DrawText("F2: +10 ecos (teste de desempenho)", x, y + 108, 12, GRAY);
    snprintf(buf, sizeof(buf), "F3: recarregar andar%d.txt", p->jogo.andar);
    DrawText(buf, x, y + 122, 12, GRAY);
}

/* ---------- telas cheias ---------- */

void desenhar_tela_menu(const Estatisticas *stats, const char *aviso_historico) {
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 10, 10, 20, 255 });

    texto_centro("O ULTIMO ANDAR", 130, 56, (Color){ 120, 200, 255, 255 });
    texto_centro("Um mago. Cinco andares. Um ultimo andar.", 205, 22,
                 (Color){ 200, 200, 220, 255 });

    char buf[160];
    snprintf(buf, sizeof(buf), "Partidas: %d     Vitorias: %d     Derrotas: %d",
             stats->partidas, stats->vitorias, stats->derrotas);
    texto_centro(buf, 280, 22, (Color){ 200, 230, 210, 255 });
    snprintf(buf, sizeof(buf), "Andar mais alto: %d     Recorde: %d pontos",
             stats->melhor_andar, stats->melhor_pontos);
    texto_centro(buf, 315, 22, (Color){ 200, 180, 255, 255 });

    if (aviso_historico != NULL && aviso_historico[0] != '\0') {
        texto_centro(aviso_historico, 352, 16, (Color){ 255, 120, 120, 255 });
    }

    Color cinza = { 150, 150, 175, 255 };
    texto_centro("WASD ou SETAS: mover", 410, 18, cinza);
    texto_centro("MOUSE: mirar     CLIQUE (segurar): lancar esferas", 435, 18, cinza);
    texto_centro("ESPACO: esquiva (atravessa os tiros)     P: pausa", 460, 18, cinza);

    texto_centro("ENTER: jogar          R: ranking", 545, 26, (Color){ 255, 255, 180, 255 });
    texto_centro("ESC para sair", 590, 20, (Color){ 150, 150, 170, 255 });
}

void desenhar_tela_nome(const char *nome) {
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 10, 10, 20, 255 });

    texto_centro("Quem vai subir a torre?", 210, 36, (Color){ 120, 200, 255, 255 });

    int largura = 420, altura = 60;
    int x = LARGURA_TELA / 2 - largura / 2, y = 290;
    DrawRectangle(x, y, largura, altura, (Color){ 28, 26, 46, 255 });
    DrawRectangleLines(x, y, largura, altura, (Color){ 140, 130, 190, 255 });

    char buf[TAM_NOME + 2];
    int cursor_visivel = ((int)(GetTime() * 2.0) % 2) == 0;
    snprintf(buf, sizeof(buf), "%s%s", nome, cursor_visivel ? "_" : "");
    DrawText(buf, x + 16, y + 14, 32, (Color){ 255, 230, 160, 255 });

    texto_centro("ENTER para comecar     ESC para voltar", 390, 22, (Color){ 255, 255, 180, 255 });
    texto_centro("(ate 15 caracteres, sem acento)", 425, 16, (Color){ 140, 140, 165, 255 });
}

void desenhar_tela_ranking(const TelaRanking *t) {
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 10, 10, 20, 255 });
    texto_centro("RANKING", 22, 40, (Color){ 255, 220, 120, 255 });

    /* abas dos critérios */
    int largura_aba = 170, x0 = LARGURA_TELA / 2 - (TOTAL_CRITERIOS * largura_aba) / 2;
    for (int i = 0; i < TOTAL_CRITERIOS; i++) {
        int sel = ((int)t->criterio == i);
        int x = x0 + i * largura_aba;
        DrawRectangle(x + 4, 74, largura_aba - 8, 28, sel ? (Color){ 70, 60, 110, 255 } : (Color){ 26, 24, 42, 255 });
        const char *nome = nome_criterio((Criterio)i);
        DrawText(nome, x + largura_aba / 2 - MeasureText(nome, 16) / 2, 80, 16,
                 sel ? (Color){ 255, 230, 160, 255 } : (Color){ 150, 150, 175, 255 });
    }

    char buf[200];
    snprintf(buf, sizeof(buf), "%s: %ld comparacoes para ordenar %d partidas",
             nome_algoritmo(t->algoritmo), t->comparacoes_ordenacao, t->total);
    texto_centro(buf, 112, 16, (Color){ 140, 255, 180, 255 });

    if (t->total == 0) {
        texto_centro("Nenhuma partida ainda. Va jogar!", 300, 26, (Color){ 200, 200, 220, 255 });
    } else {
        const int cx[7] = { 40, 85, 290, 400, 490, 580, 680 };
        const char *titulos[7] = { "#", "Nome", "Pontos", "Andar", "Tempo", "Ecos", "Data" };
        Color cor_titulo = { 130, 130, 160, 255 };
        for (int c = 0; c < 7; c++) DrawText(titulos[c], cx[c], 145, 16, cor_titulo);

        int linhas = (t->total < 12) ? t->total : 12;
        for (int i = 0; i < linhas; i++) {
            const RegistroPartida *r = &t->vista[i];
            int y = 170 + i * 27;
            int destaque = t->buscou && t->achou && comparar_nomes(r->nome, t->buscado) == 0;
            if (destaque) DrawRectangle(30, y - 4, LARGURA_TELA - 60, 25, (Color){ 60, 50, 30, 255 });
            else if (i % 2 == 0) DrawRectangle(30, y - 4, LARGURA_TELA - 60, 25, (Color){ 18, 18, 32, 255 });

            Color cor = (i == 0) ? (Color){ 255, 220, 120, 255 } : (Color){ 220, 220, 235, 255 };
            char tempo[16], data[24];
            formatar_tempo(tempo, sizeof(tempo), (float)r->tempo);
            formatar_data(data, sizeof(data), r->data);

            snprintf(buf, sizeof(buf), "%d", i + 1);            DrawText(buf, cx[0], y, 18, cor);
            DrawText(r->nome, cx[1], y, 18, cor);
            snprintf(buf, sizeof(buf), "%d", r->pontos);        DrawText(buf, cx[2], y, 18, cor);
            if (r->venceu) snprintf(buf, sizeof(buf), "TOPO");
            else           snprintf(buf, sizeof(buf), "%d", r->andar);
            DrawText(buf, cx[3], y, 18, r->venceu ? (Color){ 120, 255, 180, 255 } : cor);
            DrawText(tempo, cx[4], y, 18, cor);
            snprintf(buf, sizeof(buf), "%d", r->ecos_derrotados); DrawText(buf, cx[5], y, 18, cor);
            DrawText(data, cx[6], y, 18, cor);
        }
        if (t->total > linhas) {
            snprintf(buf, sizeof(buf), "... e mais %d partidas", t->total - linhas);
            texto_centro(buf, 170 + linhas * 27, 14, (Color){ 120, 120, 145, 255 });
        }
    }

    /* busca */
    int yb = 540;
    DrawText("Buscar jogador:", 60, yb + 8, 20, (Color){ 200, 200, 220, 255 });
    DrawRectangle(240, yb, 300, 36, (Color){ 28, 26, 46, 255 });
    DrawRectangleLines(240, yb, 300, 36, (Color){ 140, 130, 190, 255 });
    int cursor_visivel = ((int)(GetTime() * 2.0) % 2) == 0;
    snprintf(buf, sizeof(buf), "%s%s", t->busca, cursor_visivel ? "_" : "");
    DrawText(buf, 252, yb + 8, 20, (Color){ 255, 230, 160, 255 });
    DrawText("ENTER busca", 556, yb + 10, 16, (Color){ 140, 140, 165, 255 });

    if (t->buscou) {
        if (t->achou) {
            snprintf(buf, sizeof(buf), "%s: %d partida(s), %d vitoria(s), recorde %d pontos, andar mais alto %d",
                     t->buscado, t->partidas, t->vitorias, t->melhor_pontos, t->melhor_andar);
            texto_centro(buf, yb + 50, 18, (Color){ 255, 220, 120, 255 });
        } else {
            snprintf(buf, sizeof(buf), "Nenhuma partida de \"%s\".", t->buscado);
            texto_centro(buf, yb + 50, 18, (Color){ 255, 140, 140, 255 });
        }
        snprintf(buf, sizeof(buf), "Busca binaria (recursiva): %ld comparacoes     Busca linear: %ld comparacoes",
                 t->comparacoes_binaria, t->comparacoes_linear);
        texto_centro(buf, yb + 78, 16, (Color){ 140, 255, 180, 255 });
    }

    texto_centro("SETAS ESQ/DIR: criterio     TAB: Merge Sort / Quick Sort     ESC: voltar",
                 ALTURA_TELA - 30, 16, (Color){ 130, 130, 150, 255 });
}

void desenhar_tela_pausa(void) {
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, 0.6f));
    texto_centro("PAUSADO", ALTURA_TELA / 2 - 50, 56, (Color){ 220, 220, 255, 255 });
    texto_centro("P para continuar     ESC para sair", ALTURA_TELA / 2 + 20, 22,
                 (Color){ 200, 200, 220, 255 });
}

Rectangle retangulo_carta(int i) {
    const float largura = 250.0f, altura = 190.0f, espaco = 30.0f;
    float total = OFERTAS_MELHORIA * largura + (OFERTAS_MELHORIA - 1) * espaco;
    float x0 = (LARGURA_TELA - total) / 2.0f;
    return (Rectangle){ x0 + i * (largura + espaco), ALTURA_TELA / 2.0f - 90.0f, largura, altura };
}

void desenhar_tela_melhoria(const Partida *p) {
    const Jogo *j = &p->jogo;
    const Personagem *m = &p->mago;
    char buf[128];

    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, 0.78f));

    snprintf(buf, sizeof(buf), "ANDAR %d LIMPO!", j->andar);
    texto_centro(buf, 110, 44, (Color){ 120, 255, 180, 255 });
    texto_centro("Escolha uma melhoria antes de subir (tecla 1, 2, 3 ou clique)", 170, 20,
                 (Color){ 200, 200, 220, 255 });
    texto_centro("Subir a escada tambem recupera 5 de vida.", 198, 16,
                 (Color){ 150, 200, 160, 255 });

    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < OFERTAS_MELHORIA; i++) {
        Rectangle r = retangulo_carta(i);
        int em_cima = CheckCollisionPointRec(mouse, r);

        DrawRectangleRec(r, em_cima ? (Color){ 50, 44, 80, 255 } : (Color){ 32, 28, 52, 255 });
        DrawRectangleLines((int)r.x, (int)r.y, (int)r.width, (int)r.height,
                           em_cima ? (Color){ 255, 220, 120, 255 } : (Color){ 110, 100, 150, 255 });

        snprintf(buf, sizeof(buf), "[%d]", i + 1);
        DrawText(buf, (int)r.x + 12, (int)r.y + 10, 18, (Color){ 160, 150, 200, 255 });

        const char *nome = nome_melhoria(j->ofertas[i]);
        DrawText(nome, (int)(r.x + r.width / 2 - MeasureText(nome, 26) / 2.0f),
                 (int)r.y + 50, 26, (Color){ 255, 230, 160, 255 });
        DrawText(descricao_melhoria(j->ofertas[i]), (int)r.x + 22, (int)r.y + 100, 20,
                 (Color){ 220, 220, 235, 255 });
    }

    snprintf(buf, sizeof(buf), "Vida %d/%d    Dano %d    Recarga %.2fs    Velocidade %.0f    Esquiva %.2fs",
             m->vida, m->vida_max, m->ataque, m->tempo_entre_esferas, m->velocidade,
             m->tempo_recarga_esquiva);
    texto_centro(buf, ALTURA_TELA / 2 + 130, 18, (Color){ 160, 160, 190, 255 });
}

/* Pontos, posição no ranking e as últimas linhas do diário (telas de fim). */
static void desenhar_resumo_final(const Partida *p, int y) {
    const Jogo *j = &p->jogo;
    char buf[160], linha[TAM_LINHA_DIARIO];

    if (j->posicao > 0) {
        snprintf(buf, sizeof(buf), "%d pontos  -  posicao %d de %d no ranking",
                 j->pontos, j->posicao, j->total_partidas);
    } else {
        snprintf(buf, sizeof(buf), "%d pontos", j->pontos);
    }
    texto_centro(buf, y, 28, (Color){ 255, 220, 120, 255 });

    if (p->qtd_resumo > 0) {
        int caixa_y = y + 70;
        DrawRectangle(110, caixa_y - 10, LARGURA_TELA - 220, 30 + p->qtd_resumo * 24, Fade(BLACK, 0.35f));
        DrawText("Diario de combate (assets/dados/diario.txt):", 125, caixa_y - 2, 14,
                 (Color){ 140, 140, 165, 255 });
        for (int i = 0; i < p->qtd_resumo; i++) {
            texto_que_cabe(p->resumo[i], linha, sizeof(linha), LARGURA_TELA - 260, 16);
            DrawText(linha, 125, caixa_y + 20 + i * 24, 16, (Color){ 210, 210, 225, 255 });
        }
    }
}

void desenhar_tela_vitoria(const Estatisticas *stats, const Partida *p) {
    const Jogo *j = &p->jogo;
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 5, 15, 25, 255 });

    texto_centro("VITORIA!", 50, 72, (Color){ 120, 255, 180, 255 });
    texto_centro("Voce chegou ao topo da torre.", 135, 26, (Color){ 220, 220, 240, 255 });

    char buf[160], tempo[16];
    formatar_tempo(tempo, sizeof(tempo), j->tempo_partida);
    snprintf(buf, sizeof(buf), "Tempo: %s     Ecos derrotados: %d     Vitorias totais: %d",
             tempo, j->ecos_derrotados, stats->vitorias);
    texto_centro(buf, 180, 20, (Color){ 200, 200, 255, 255 });

    desenhar_resumo_final(p, 230);

    texto_centro("Pressione ENTER para voltar ao menu", ALTURA_TELA - 70, 22,
                 (Color){ 255, 255, 180, 255 });
}

void desenhar_tela_derrota(const Estatisticas *stats, const Partida *p) {
    const Jogo *j = &p->jogo;
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 25, 5, 5, 255 });

    texto_centro("VOCE CAIU", 50, 72, (Color){ 255, 100, 100, 255 });
    texto_centro("O Ultimo Andar guarda mais um nome.", 135, 26, (Color){ 240, 200, 200, 255 });

    char buf[160], tempo[16];
    formatar_tempo(tempo, sizeof(tempo), j->tempo_partida);
    snprintf(buf, sizeof(buf), "Caiu no andar %d (%s) depois de %s     Derrotas totais: %d",
             j->andar, p->andar.nome, tempo, stats->derrotas);
    texto_centro(buf, 180, 20, (Color){ 255, 180, 180, 255 });

    desenhar_resumo_final(p, 230);

    texto_centro("Pressione ENTER para voltar ao menu", ALTURA_TELA - 70, 22,
                 (Color){ 255, 255, 180, 255 });
}
