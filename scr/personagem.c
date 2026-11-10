#include "personagem.h"
#include "mapa.h"
#include "projetil.h"
#include "vetor.h"
#include <raylib.h>
#include <string.h>

/* Ajuste fino da jogabilidade: mexa nesses números para mudar a sensação. */
#define VELOCIDADE_MAGO       190.0f   /* px/s */
#define RAIO_MAGO              15.0f
#define TEMPO_ENTRE_ESFERAS     0.35f  /* recarga da esfera, em segundos */
#define VELOCIDADE_ESFERA     520.0f
#define RAIO_ESFERA             9.0f
#define DURACAO_ESFERA          0.9f   /* alcance = velocidade x duração */
#define DURACAO_ESQUIVA         0.18f
#define MULT_ESQUIVA            3.2f   /* quantas vezes mais rápido na esquiva */
#define RECARGA_ESQUIVA         0.9f   /* segundos entre uma esquiva e outra */
#define TEMPO_INVULNERAVEL      0.9f   /* depois de levar dano */
#define FORCA_EMPURRAO        320.0f
#define ATRITO_EMPURRAO         9.0f

Personagem criar_mago(Vector2 pos) {
    Personagem p;
    memset(&p, 0, sizeof(p));
    strncpy(p.nome, "Mago", sizeof(p.nome) - 1);
    p.pos                 = pos;
    p.mira                = (Vector2){ 1.0f, 0.0f };
    p.raio                = RAIO_MAGO;
    p.velocidade          = VELOCIDADE_MAGO;
    p.vida                = 20;
    p.vida_max            = 20;
    p.ataque              = 5;
    p.tempo_entre_esferas = TEMPO_ENTRE_ESFERAS;
    p.tempo_recarga_esquiva = RECARGA_ESQUIVA;
    return p;
}

Controles ler_controles(void) {
    Controles c = { 0 };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    c.direcao.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  c.direcao.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  c.direcao.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) c.direcao.x += 1.0f;

    /* Normalizar impede que andar na diagonal seja mais rápido. */
    c.direcao = vnormaliza(c.direcao);

    Vector2 mouse = GetMousePosition();
    c.alvo     = (Vector2){ mouse.x, mouse.y - ALTURA_HUD };  /* tela -> sala */
    c.atirar   = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    c.esquivar = IsKeyPressed(KEY_SPACE);
    return c;
}

void atualizar_mago(Personagem *p, Controles c, const Mapa *m,
                    GerenciadorProjeteis *projeteis, float dt) {
    if (p == NULL || m == NULL) return;

    reduzir_tempo(&p->recarga_esfera, dt);
    reduzir_tempo(&p->recarga_esquiva, dt);
    reduzir_tempo(&p->invulneravel, dt);
    reduzir_tempo(&p->esquivando, dt);

    /* Mira: aponta para o mouse. */
    Vector2 para_alvo = vsub(c.alvo, p->pos);
    if (vtamanho(para_alvo) > 1.0f) p->mira = vnormaliza(para_alvo);

    /* Esquiva: arrancada curta na direção do movimento (ou da mira, se parado). */
    if (c.esquivar && p->recarga_esquiva <= 0.0f) {
        int parado = (c.direcao.x == 0.0f && c.direcao.y == 0.0f);
        p->dir_esquiva     = parado ? p->mira : c.direcao;
        p->esquivando      = DURACAO_ESQUIVA;
        p->recarga_esquiva = p->tempo_recarga_esquiva;
    }

    Vector2 vel;
    if (p->esquivando > 0.0f) vel = vescala(p->dir_esquiva, p->velocidade * MULT_ESQUIVA);
    else                      vel = vescala(c.direcao, p->velocidade);

    vel = vsoma(vel, p->empurrao);
    p->empurrao = amortecer(p->empurrao, ATRITO_EMPURRAO, dt);

    /* posição nova = posição + velocidade x tempo do quadro */
    mover_com_colisao(m, &p->pos, vescala(vel, dt), p->raio);

    /* Ataque: segurando o botão, lança uma esfera a cada recarga. */
    if (c.atirar && p->recarga_esfera <= 0.0f && projeteis != NULL) {
        Vector2 saida = vsoma(p->pos, vescala(p->mira, p->raio));
        disparar_projetil(projeteis, saida, vescala(p->mira, VELOCIDADE_ESFERA),
                          RAIO_ESFERA, p->ataque, 1, DURACAO_ESFERA);
        p->recarga_esfera = p->tempo_entre_esferas;
    }
}

int danificar_mago(Personagem *p, int dano, Vector2 origem) {
    if (p == NULL) return 0;
    if (p->invulneravel > 0.0f || p->esquivando > 0.0f) return 0;

    p->vida -= dano;
    if (p->vida < 0) p->vida = 0;
    p->invulneravel = TEMPO_INVULNERAVEL;

    Vector2 para_longe = vnormaliza(vsub(p->pos, origem));
    p->empurrao = vescala(para_longe, FORCA_EMPURRAO);
    return 1;
}

void desenhar_personagem(const Personagem *p, const Assets *a) {
    if (p == NULL || a == NULL) return;

    /* sombra no chão */
    DrawEllipse((int)p->pos.x, (int)(p->pos.y + 17), 15, 5, Fade(BLACK, 0.45f));

    /* pontinho que mostra para onde a esfera vai */
    Vector2 ponta = vsoma(p->pos, vescala(p->mira, 34.0f));
    DrawCircleV(ponta, 4.0f, (Color){ 120, 180, 255, 200 });

    /* Depois de levar dano ele pisca enquanto estiver invulnerável. */
    if (p->invulneravel > 0.0f && ((int)(p->invulneravel * 16.0f) % 2) == 1) return;

    Rectangle origem = { 0, 0, (float)a->mago.width, (float)a->mago.height };
    if (p->mira.x < 0.0f) origem.width = -origem.width;   /* vira para a esquerda */

    Rectangle destino = { p->pos.x - TAM_TILE / 2.0f, p->pos.y - TAM_TILE / 2.0f - 4.0f,
                          TAM_TILE, TAM_TILE };

    Color tinta = (p->esquivando > 0.0f) ? (Color){ 150, 200, 255, 150 } : WHITE;
    DrawTexturePro(a->mago, origem, destino, (Vector2){ 0, 0 }, 0.0f, tinta);
}
