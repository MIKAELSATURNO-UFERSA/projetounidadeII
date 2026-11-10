#include "projetil.h"
#include "mapa.h"
#include "equipe.h"
#include "personagem.h"
#include "efeitos.h"
#include "vetor.h"
#include <raylib.h>

#define EMPURRAO_ESFERA 260.0f   /* o quanto a esfera empurra o eco */

static const Color COR_ESFERA     = { 120, 180, 255, 255 };
static const Color COR_TIRO_ECO   = { 200,  90, 240, 255 };
static const Color COR_MORTE_ECO  = { 120, 255, 200, 255 };

void inicializar_projeteis(GerenciadorProjeteis *g) {
    if (g == NULL) return;
    for (int i = 0; i < MAX_PROJETEIS; i++) g->itens[i].ativo = 0;
}

void disparar_projetil(GerenciadorProjeteis *g, Vector2 pos, Vector2 vel,
                       float raio, int dano, int do_jogador, float duracao) {
    if (g == NULL) return;
    for (int i = 0; i < MAX_PROJETEIS; i++) {
        if (!g->itens[i].ativo) {
            Projetil *p   = &g->itens[i];
            p->pos        = pos;
            p->vel        = vel;
            p->raio       = raio;
            p->dano       = dano;
            p->do_jogador = do_jogador;
            p->tempo_vida = duracao;
            p->ativo      = 1;
            return;
        }
    }
    /* Vetor cheio: o disparo é ignorado (com 128 vagas isso quase não acontece). */
}

int atualizar_projeteis(GerenciadorProjeteis *g, const Mapa *m,
                        Personagem *mago, EquipeEcos *ecos,
                        GerenciadorEfeitos *efeitos, float dt,
                        int *ecos_destruidos, int *lembrancas_destruidas) {
    if (g == NULL || m == NULL) return 0;
    int dano_no_mago = 0;

    for (int i = 0; i < MAX_PROJETEIS; i++) {
        Projetil *p = &g->itens[i];
        if (!p->ativo) continue;

        p->pos = vsoma(p->pos, vescala(p->vel, dt));
        p->tempo_vida -= dt;
        if (p->tempo_vida <= 0.0f) { p->ativo = 0; continue; }

        if (circulo_colide_parede(m, p->pos, p->raio * 0.5f)) {
            adicionar_impacto(efeitos, p->pos,
                              p->do_jogador ? COR_ESFERA : COR_TIRO_ECO, 14.0f);
            p->ativo = 0;
            continue;
        }

        if (p->do_jogador && ecos != NULL) {
            /* Busca linear: testa a esfera contra cada eco. */
            for (int j = 0; j < ecos->total; j++) {
                Eco *e = &ecos->itens[j];
                if (e->surgindo > 0.0f) continue;
                if (vdistancia(p->pos, e->pos) >= p->raio + e->raio) continue;

                e->vida    -= p->dano;
                e->flash    = 0.12f;
                e->empurrao = vescala(vnormaliza(p->vel), EMPURRAO_ESFERA);
                adicionar_mensagem(efeitos, e->pos, -p->dano);
                adicionar_impacto(efeitos, p->pos, COR_ESFERA, 18.0f);

                if (e->vida <= 0) {
                    adicionar_impacto(efeitos, e->pos, COR_MORTE_ECO, 40.0f);
                    if (e->lembranca && lembrancas_destruidas != NULL) (*lembrancas_destruidas)++;
                    remover_eco(ecos, j);
                    if (ecos_destruidos != NULL) (*ecos_destruidos)++;
                }
                p->ativo = 0;
                break;   /* uma esfera acerta um eco só */
            }

        } else if (!p->do_jogador && mago != NULL) {
            if (mago->esquivando > 0.0f) continue;   /* esquiva atravessa o tiro */
            if (vdistancia(p->pos, mago->pos) >= p->raio + mago->raio) continue;

            /* "origem" atrás do mago, na direção de onde o tiro veio: empurra no sentido do tiro */
            Vector2 origem = vsub(mago->pos, vnormaliza(p->vel));
            if (danificar_mago(mago, p->dano, origem)) {
                dano_no_mago += p->dano;
                adicionar_mensagem(efeitos, mago->pos, -p->dano);
            }
            adicionar_impacto(efeitos, p->pos, COR_TIRO_ECO, 16.0f);
            p->ativo = 0;
        }
    }
    return dano_no_mago;
}

int contar_projeteis_ativos(const GerenciadorProjeteis *g) {
    if (g == NULL) return 0;
    int n = 0;
    for (int i = 0; i < MAX_PROJETEIS; i++) n += g->itens[i].ativo;
    return n;
}

void desenhar_projeteis(const GerenciadorProjeteis *g) {
    if (g == NULL) return;
    for (int i = 0; i < MAX_PROJETEIS; i++) {
        const Projetil *p = &g->itens[i];
        if (!p->ativo) continue;

        if (p->do_jogador) {
            DrawCircleV(p->pos, p->raio * 1.3f, (Color){ 120, 180, 255, 90 });
            DrawCircleV(p->pos, p->raio,        (Color){ 150, 200, 255, 220 });
            DrawCircleV(p->pos, p->raio * 0.5f, WHITE);
        } else {
            DrawCircleV(p->pos, p->raio,        (Color){ 140,  40, 180, 210 });
            DrawCircleV(p->pos, p->raio * 0.6f, (Color){ 230, 140, 255, 255 });
            DrawCircleV(p->pos, p->raio * 0.3f, WHITE);
        }
    }
}
