#include "equipe.h"
#include "mapa.h"
#include "personagem.h"
#include "projetil.h"
#include "efeitos.h"
#include "vetor.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CAPACIDADE_INICIAL_ECOS 4

#define TEMPO_SURGINDO      0.8f
#define DIST_MIN_ATIRADOR 170.0f   /* mais perto que isso, o atirador recua */
#define DIST_MAX_ATIRADOR 270.0f   /* mais longe que isso, ele se aproxima */
#define ALCANCE_TIRO      430.0f
#define VELOCIDADE_TIRO   230.0f
#define RAIO_TIRO           7.0f
#define DURACAO_TIRO        2.2f
#define ATRITO_EMPURRAO     8.0f
#define RECUO_CONTATO     200.0f   /* eco "quica" para trás quando acerta o mago */

/* ===================== vetor dinâmico ===================== */

void inicializar_equipe_ecos(EquipeEcos *eq) {
    if (eq == NULL) return;
    eq->capacidade = CAPACIDADE_INICIAL_ECOS;
    eq->total = 0;
    eq->itens = (Eco *) calloc(eq->capacidade, sizeof(Eco));
    if (eq->itens == NULL) {
        TraceLog(LOG_ERROR, "Falha ao alocar equipe de ecos.");
        eq->capacidade = 0;
    }
}

void liberar_equipe_ecos(EquipeEcos *eq) {
    if (eq == NULL) return;
    free(eq->itens);
    eq->itens = NULL;
    eq->total = 0;
    eq->capacidade = 0;
}

static int aumentar_capacidade(EquipeEcos *eq, int nova_capacidade) {
    if (eq == NULL) return 0;
    if (nova_capacidade <= eq->capacidade) return 0;

    Eco *temp = (Eco *) realloc(eq->itens, nova_capacidade * sizeof(Eco));
    if (temp == NULL) {
        TraceLog(LOG_ERROR, "Falha no realloc de ecos.");
        return 0;
    }

    /* zera só a parte nova do vetor */
    memset(temp + eq->capacidade, 0,
           (nova_capacidade - eq->capacidade) * sizeof(Eco));

    eq->itens = temp;
    eq->capacidade = nova_capacidade;
    return 1;
}

int adicionar_eco(EquipeEcos *eq, Eco e) {
    if (eq == NULL) return 0;
    if (eq->total >= eq->capacidade) {
        /* Cresce de 2 em 2 (como na unidade I). Na Fase 4 a gente
           compara com dobrar a capacidade. */
        if (!aumentar_capacidade(eq, eq->capacidade + 2)) return 0;
    }
    eq->itens[eq->total] = e;
    eq->total++;
    return 1;
}

int remover_eco(EquipeEcos *eq, int indice) {
    if (eq == NULL) return 0;
    if (indice < 0 || indice >= eq->total) return 0;

    /* Desloca os seguintes uma posição para trás: O(n), mas mantém a ordem
       do vetor (importante, porque ele fica ordenado por Y para o desenho). */
    for (int i = indice; i < eq->total - 1; i++) {
        eq->itens[i] = eq->itens[i + 1];
    }
    eq->total--;
    return 1;
}

Eco criar_eco(int andar, TipoEco tipo, Vector2 pos) {
    Eco e;
    memset(&e, 0, sizeof(e));
    e.tipo     = tipo;
    e.pos      = pos;
    e.surgindo = TEMPO_SURGINDO;

    if (tipo == ECO_ATIRADOR) {
        snprintf(e.nome, sizeof(e.nome), "Eco Atirador-%d", andar);
        e.vida_max     = 6 + andar * 2;
        e.ataque       = 2 + (andar - 1) / 2;   /* 2, 2, 3, 3, 4 */
        e.velocidade   = 70.0f;
        e.raio         = 14.0f;
        e.recarga_tiro = 1.0f + GetRandomValue(0, 100) / 100.0f;
        e.sentido      = GetRandomValue(0, 1) ? 1 : -1;
    } else {
        snprintf(e.nome, sizeof(e.nome), "Eco-%d", andar);
        e.vida_max   = 8 + andar * 2;
        e.ataque     = 3 + (andar - 1) / 2;     /* 3, 3, 4, 4, 5 */
        e.velocidade = 80.0f + andar * 8.0f;
        e.raio       = 15.0f;
    }
    e.vida = e.vida_max;
    return e;
}

int eco_perto(const EquipeEcos *eq, Vector2 p, float distancia) {
    if (eq == NULL) return 0;
    for (int i = 0; i < eq->total; i++) {
        if (vdistancia(eq->itens[i].pos, p) < distancia) return 1;
    }
    return 0;
}

/* ===================== comportamento ===================== */

int atualizar_ecos(EquipeEcos *eq, Personagem *mago, const Mapa *m,
                   const MapaDistancias *md,
                   GerenciadorProjeteis *projeteis, GerenciadorEfeitos *efeitos,
                   Desempenho *d, float dt) {
    if (eq == NULL || mago == NULL || m == NULL || md == NULL) return 0;
    int dano_no_mago = 0;

    for (int i = 0; i < eq->total; i++) {
        Eco *e = &eq->itens[i];
        reduzir_tempo(&e->flash, dt);

        if (e->surgindo > 0.0f) {          /* ainda aparecendo: fica parado */
            reduzir_tempo(&e->surgindo, dt);
            continue;
        }

        Vector2 para_mago = vsub(mago->pos, e->pos);
        float   dist      = vtamanho(para_mago);
        Vector2 dir       = vnormaliza(para_mago);
        Vector2 vel       = { 0.0f, 0.0f };

        /*
         * Para onde andar quando quer chegar no mago: se o caminho reto está
         * livre, vai reto; senão, segue o mapa de distâncias e contorna as paredes.
         */
        Vector2 rumo = dir;
        if (!caminho_livre(m, e->pos, mago->pos, e->raio)) {
            Vector2 pelo_mapa = direcao_pelo_mapa(m, md, e->pos);
            if (pelo_mapa.x != 0.0f || pelo_mapa.y != 0.0f) rumo = pelo_mapa;
        }

        int andando_de_lado = 0;

        if (e->tipo == ECO_CACADOR) {
            vel = vescala(rumo, e->velocidade);
        } else {
            /* "tem tiro" = o projétil inteiro passa sem raspar em nenhuma parede */
            int tem_tiro = (dist < ALCANCE_TIRO) &&
                           caminho_livre(m, e->pos, mago->pos, RAIO_TIRO);

            if (!tem_tiro || dist > DIST_MAX_ATIRADOR) {
                vel = vescala(rumo, e->velocidade);          /* procura posição de tiro */
            } else if (dist < DIST_MIN_ATIRADOR) {
                vel = vescala(dir, -e->velocidade);          /* recua */
            } else {
                /* na distância boa: anda de lado, rodeando o mago */
                Vector2 lado = { -dir.y * (float)e->sentido, dir.x * (float)e->sentido };
                vel = vescala(lado, e->velocidade * 0.6f);
                andando_de_lado = 1;
            }

            e->recarga_tiro -= dt;
            if (e->recarga_tiro <= 0.0f) {
                if (tem_tiro) {
                    disparar_projetil(projeteis, e->pos, vescala(dir, VELOCIDADE_TIRO),
                                      RAIO_TIRO, e->ataque, 0, DURACAO_TIRO);
                    e->recarga_tiro = 1.6f + GetRandomValue(0, 60) / 100.0f;
                } else {
                    e->recarga_tiro = 0.2f;   /* sem tiro limpo: tenta de novo daqui a pouco */
                }
            }
        }

        /*
         * Separação: cada eco compara a posição com TODOS os outros para não
         * ficarem um em cima do outro. São n x (n-1) testes por quadro -> O(n²).
         *
         * Quem está mais perto do mago tem a preferência: o de trás é que
         * se afasta. Sem isso, dois ecos chegando juntos numa passagem
         * estreita se empurram para fora dela e travam ali para sempre.
         */
        Vector2 separa = { 0.0f, 0.0f };
        for (int j = 0; j < eq->total; j++) {
            if (j == i) continue;
            if (d != NULL) d->testes_separacao++;

            const Eco *o = &eq->itens[j];
            Vector2 afasta = vsub(e->pos, o->pos);
            float   dd     = vtamanho(afasta);
            float   minimo = e->raio + o->raio;
            if (dd >= minimo) continue;

            float dist_outro = vdistancia(o->pos, mago->pos);
            float peso = 0.5f;                                  /* empate: cada um se afasta metade */
            if (dist_outro < dist - 1.0f)      peso = 1.0f;     /* o outro está na frente: eu cedo */
            else if (dist_outro > dist + 1.0f) peso = 0.0f;     /* eu estou na frente: ele que ceda */

            if (dd < 0.01f) afasta = (Vector2){ (i < j) ? -1.0f : 1.0f, 0.0f };
            separa = vsoma(separa, vescala(vnormaliza(afasta), (minimo - dd) * peso));
        }

        vel = vsoma(vel, e->empurrao);
        e->empurrao = amortecer(e->empurrao, ATRITO_EMPURRAO, dt);

        int bateu = mover_com_colisao(m, &e->pos, vsoma(vescala(vel, dt), separa), e->raio);
        if (bateu && andando_de_lado) e->sentido = -e->sentido;   /* bateu na parede: volta */

        /* Contato com o mago */
        if (vdistancia(e->pos, mago->pos) < e->raio + mago->raio) {
            if (danificar_mago(mago, e->ataque, e->pos)) {
                dano_no_mago += e->ataque;
                adicionar_mensagem(efeitos, mago->pos, -e->ataque);
                e->empurrao = vescala(dir, -RECUO_CONTATO);
            }
        }
    }
    return dano_no_mago;
}

/* ===================== desenho ===================== */

void desenhar_eco(const Eco *e, const Assets *a) {
    if (e == NULL || a == NULL) return;

    float alfa = 1.0f;
    if (e->surgindo > 0.0f) {
        /* Aviso de onde o eco vai aparecer: um anel que fecha. */
        float t = 1.0f - e->surgindo / TEMPO_SURGINDO;     /* 0 -> 1 */
        DrawCircleLines((int)e->pos.x, (int)e->pos.y, e->raio * (2.6f - 1.6f * t),
                        Fade((Color){ 200, 120, 255, 255 }, 0.4f + 0.6f * t));
        alfa = 0.15f + 0.5f * t;
    }

    DrawEllipse((int)e->pos.x, (int)(e->pos.y + 16), 14, 5, Fade(BLACK, 0.4f * alfa));

    /* O atirador ganha uma cor diferente para o jogador reconhecer de longe;
       o eco lembrança é dourado e um pouco maior. */
    Color tinta = WHITE;
    if (e->tipo == ECO_ATIRADOR) tinta = (Color){ 210, 150, 255, 255 };
    if (e->lembranca)            tinta = (Color){ 255, 210, 110, 255 };
    float tam = e->lembranca ? TAM_TILE * 1.25f : (float)TAM_TILE;

    if (e->lembranca && e->surgindo <= 0.0f) {   /* brilho em volta */
        DrawCircleV(e->pos, e->raio + 10.0f, Fade((Color){ 255, 210, 110, 255 }, 0.15f));
    }

    Rectangle origem  = { 0, 0, (float)a->eco.width, (float)a->eco.height };
    Rectangle destino = { e->pos.x - tam / 2.0f, e->pos.y - tam / 2.0f - 4.0f, tam, tam };
    DrawTexturePro(a->eco, origem, destino, (Vector2){ 0, 0 }, 0.0f, Fade(tinta, alfa));

    if (e->lembranca) {   /* nome de quem caiu aqui */
        int largura_nome = MeasureText(e->nome, 14);
        int nx = (int)(e->pos.x - largura_nome / 2.0f);
        int ny = (int)(e->pos.y - tam / 2.0f - 26.0f);
        DrawRectangle(nx - 4, ny - 2, largura_nome + 8, 18, Fade(BLACK, 0.6f * alfa));
        DrawText(e->nome, nx, ny, 14, Fade((Color){ 255, 220, 140, 255 }, alfa));
    }

    if (e->flash > 0.0f) {   /* brilho branco quando leva dano */
        DrawCircleV((Vector2){ e->pos.x, e->pos.y - 4.0f }, e->raio + 3.0f, Fade(WHITE, 0.55f));
    }

    /* Barra de vida só aparece depois do primeiro acerto. */
    if (e->surgindo <= 0.0f && e->vida < e->vida_max) {
        int largura = 36;
        int bx = (int)(e->pos.x - largura / 2.0f);
        int by = (int)(e->pos.y - tam / 2.0f - 6.0f);
        float pct = (float)e->vida / e->vida_max;
        DrawRectangle(bx, by, largura, 5, (Color){ 60, 20, 60, 255 });
        DrawRectangle(bx, by, (int)(largura * pct), 5, (Color){ 220, 80, 220, 255 });
    }
}
