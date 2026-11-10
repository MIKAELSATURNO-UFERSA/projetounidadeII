#include "efeitos.h"
#include <raylib.h>
#include <stdio.h>

#define DURACAO_MENSAGEM  0.9f
#define DURACAO_IMPACTO   0.25f

void inicializar_efeitos(GerenciadorEfeitos *g) {
    if (g == NULL) return;
    for (int i = 0; i < MAX_MENSAGENS; i++) g->mensagens[i].ativo = 0;
    for (int i = 0; i < MAX_IMPACTOS;  i++) g->impactos[i].ativo  = 0;
}

void adicionar_mensagem(GerenciadorEfeitos *g, Vector2 pos, int valor) {
    if (g == NULL) return;
    for (int i = 0; i < MAX_MENSAGENS; i++) {
        MensagemDano *m = &g->mensagens[i];
        if (!m->ativo) {
            m->pos            = pos;
            m->valor          = valor;
            m->tempo_restante = DURACAO_MENSAGEM;
            m->tempo_total    = DURACAO_MENSAGEM;
            m->ativo          = 1;
            return;
        }
    }
}

void adicionar_impacto(GerenciadorEfeitos *g, Vector2 pos, Color cor, float tamanho) {
    if (g == NULL) return;
    for (int i = 0; i < MAX_IMPACTOS; i++) {
        Impacto *im = &g->impactos[i];
        if (!im->ativo) {
            im->pos            = pos;
            im->cor            = cor;
            im->tamanho        = tamanho;
            im->tempo_restante = DURACAO_IMPACTO;
            im->tempo_total    = DURACAO_IMPACTO;
            im->ativo          = 1;
            return;
        }
    }
}

void atualizar_efeitos(GerenciadorEfeitos *g, float dt) {
    if (g == NULL) return;
    for (int i = 0; i < MAX_MENSAGENS; i++) {
        if (!g->mensagens[i].ativo) continue;
        g->mensagens[i].tempo_restante -= dt;
        if (g->mensagens[i].tempo_restante <= 0.0f) g->mensagens[i].ativo = 0;
    }
    for (int i = 0; i < MAX_IMPACTOS; i++) {
        if (!g->impactos[i].ativo) continue;
        g->impactos[i].tempo_restante -= dt;
        if (g->impactos[i].tempo_restante <= 0.0f) g->impactos[i].ativo = 0;
    }
}

void desenhar_efeitos(const GerenciadorEfeitos *g) {
    if (g == NULL) return;

    for (int i = 0; i < MAX_IMPACTOS; i++) {
        const Impacto *im = &g->impactos[i];
        if (!im->ativo) continue;

        float t = 1.0f - im->tempo_restante / im->tempo_total;   /* 0 -> 1 */
        Color cor = Fade(im->cor, 1.0f - t);
        DrawCircleLines((int)im->pos.x, (int)im->pos.y, im->tamanho * (0.4f + t), cor);
        DrawCircleV(im->pos, im->tamanho * 0.35f * (1.0f - t), cor);
    }

    for (int i = 0; i < MAX_MENSAGENS; i++) {
        const MensagemDano *m = &g->mensagens[i];
        if (!m->ativo) continue;

        float t = 1.0f - m->tempo_restante / m->tempo_total;
        char buf[16];
        if (m->valor < 0) snprintf(buf, sizeof(buf), "%d", m->valor);
        else              snprintf(buf, sizeof(buf), "+%d", m->valor);

        Color cor = (m->valor < 0) ? (Color){ 255, 90, 90, 255 }
                                   : (Color){ 80, 255, 120, 255 };
        cor = Fade(cor, 1.0f - t);

        int tam = 22;
        int largura = MeasureText(buf, tam);
        DrawText(buf, (int)(m->pos.x - largura / 2.0f),
                 (int)(m->pos.y - 34.0f - t * 30.0f), tam, cor);
    }
}
