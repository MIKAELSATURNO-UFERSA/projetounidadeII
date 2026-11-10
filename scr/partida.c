#include "partida.h"
#include "andar.h"
#include "mapa.h"
#include "equipe.h"
#include "projetil.h"
#include "efeitos.h"
#include "onda.h"
#include "melhoria.h"
#include "ordenacao.h"
#include "busca.h"
#include "historico.h"
#include "diario.h"
#include "vetor.h"
#include <raylib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define TEMPO_INTERVALO  3.0f    /* segundos de respiro entre as ondas */
#define TEMPO_TITULO     2.0f    /* nome do andar na tela antes da 1a onda */
#define TEMPO_TREMOR     0.2f
#define DURACAO_MENSAGEM 4.0f
#define RAIO_ESCADA     22.0f    /* o quanto o mago precisa chegar perto da escada */
#define CURA_ESCADA         5    /* vida recuperada ao subir de andar */
#define BONUS_LEMBRANCA   100    /* pontos por derrotar um eco lembrança */

void registrar_evento(Partida *p, const char *formato, ...) {
    if (p == NULL || formato == NULL) return;

    va_list args;
    va_start(args, formato);
    vsnprintf(p->jogo.mensagem, sizeof(p->jogo.mensagem), formato, args);
    va_end(args);

    p->jogo.tempo_mensagem = DURACAO_MENSAGEM;

    /* Todo acontecimento também vai para o diário de combate (arquivo texto). */
    diario_escrever(p->jogo.tempo_partida, p->jogo.andar, p->jogo.mensagem);
}

Vector2 posicao_escada(const Partida *p) {
    return centro_do_tile(p->andar.escada_x, p->andar.escada_y);
}

/*
 * Prepara a sala de um andar: lê o arquivo, põe o mago no 'M' e limpa a sala.
 * A vida e as melhorias do mago continuam de um andar para o outro.
 */
static void entrar_no_andar(Partida *p, int numero) {
    Jogo *j = &p->jogo;

    carregar_andar(numero, &p->andar, p->aviso_andar, sizeof(p->aviso_andar));

    j->andar          = numero;
    j->onda           = 0;               /* a 1a onda começa quando o título some */
    j->ondas_no_andar = p->andar.ondas;
    j->escada_aberta  = 0;
    j->intervalo      = TEMPO_TITULO;
    j->tempo_titulo   = TEMPO_TITULO + 0.5f;

    p->mago.pos          = centro_do_tile(p->andar.inicio_x, p->andar.inicio_y);
    p->mago.empurrao     = (Vector2){ 0.0f, 0.0f };
    p->mago.esquivando   = 0.0f;
    p->mago.invulneravel = 0.0f;
    p->ecos.total        = 0;
    p->tremor            = 0.0f;
    inicializar_projeteis(&p->projeteis);
    inicializar_efeitos(&p->efeitos);
    calcular_distancias(&p->andar.mapa, p->mago.pos, &p->distancias);

    if (p->aviso_andar[0] != '\0') {
        registrar_evento(p, "%s", p->aviso_andar);
    } else {
        registrar_evento(p, "Andar %d: %s", numero, p->andar.nome);
    }
}

void preparar_partida(Partida *p, Historico *h) {
    if (p == NULL) return;
    memset(p, 0, sizeof(*p));
    p->historico = h;

    inicializar_equipe_ecos(&p->ecos);
    inicializar_projeteis(&p->projeteis);
    inicializar_efeitos(&p->efeitos);

    p->mago           = criar_mago((Vector2){ 0.0f, 0.0f });
    p->jogo.andar_max = TOTAL_ANDARES;
    p->jogo.estado    = ESTADO_MENU;
}

void reiniciar_partida(Partida *p, const char *jogador) {
    if (p == NULL) return;

    int debug = p->jogo.debug;          /* o modo debug sobrevive ao reinício */
    memset(&p->jogo, 0, sizeof(p->jogo));
    p->jogo.estado    = ESTADO_JOGANDO;
    p->jogo.andar_max = TOTAL_ANDARES;
    p->jogo.debug     = debug;
    strncpy(p->jogo.jogador, (jogador != NULL && jogador[0] != '\0') ? jogador : "Mago",
            TAM_NOME - 1);

    p->mago = criar_mago((Vector2){ 0.0f, 0.0f });
    memset(&p->desempenho, 0, sizeof(p->desempenho));
    p->qtd_resumo = 0;

    diario_comecar_partida(p->jogo.jogador);
    entrar_no_andar(p, 1);
}

/*
 * Quando a partida acaba: monta o registro, grava no fim do historico.bin,
 * coloca no vetor na posição certa do ranking e fecha o diário.
 */
static void finalizar_partida(Partida *p, int venceu) {
    Jogo *j = &p->jogo;

    RegistroPartida r;
    memset(&r, 0, sizeof(r));
    strncpy(r.nome, j->jogador, TAM_NOME - 1);
    r.venceu          = venceu;
    r.andar           = j->andar;
    r.onda            = j->onda;
    r.tempo           = (int)j->tempo_partida;
    r.ecos_derrotados = j->ecos_derrotados;
    r.data            = (long long)time(NULL);
    r.pontos          = calcular_pontos(&r, j->bonus);

    j->pontos  = r.pontos;
    j->posicao = 0;

    if (!historico_gravar(&r)) {
        registrar_evento(p, "Nao foi possivel salvar a partida no historico.");
    }
    if (p->historico != NULL) {
        int pos = historico_inserir_ordenado(p->historico, r);
        if (pos >= 0) j->posicao = pos + 1;
        j->total_partidas = p->historico->total;
    }

    diario_terminar_partida(venceu, j->andar, r.pontos);
    p->qtd_resumo = diario_ultimas_linhas(p->resumo, MAX_LINHAS_RESUMO);

    j->estado = venceu ? ESTADO_VITORIA : ESTADO_DERROTA;
}

/*
 * Eco lembrança: se alguém já caiu neste andar em outra partida, o eco
 * dessa pessoa aparece aqui, mais forte e com o nome dela.
 *
 * 1) separa só as derrotas do histórico;
 * 2) ordena por andar (Quick Sort com pivô aleatório);
 * 3) acha uma derrota deste andar com busca binária;
 * 4) como estão ordenadas, as outras do mesmo andar estão do lado: sorteia uma.
 */
static void spawnar_lembranca(Partida *p) {
    const Historico *h = p->historico;
    if (h == NULL || h->total == 0) return;

    RegistroPartida *quedas = (RegistroPartida *) malloc(h->total * sizeof(RegistroPartida));
    if (quedas == NULL) return;

    int n = 0;
    for (int i = 0; i < h->total; i++) {
        if (!h->itens[i].venceu) quedas[n++] = h->itens[i];
    }

    quick_sort_registros(quedas, n, CRITERIO_ANDAR, NULL);
    int achado = busca_binaria_andar(quedas, n, p->jogo.andar, NULL);

    if (achado >= 0) {
        int ini = achado, fim = achado;
        while (ini > 0 && quedas[ini - 1].andar == p->jogo.andar) ini--;
        while (fim < n - 1 && quedas[fim + 1].andar == p->jogo.andar) fim++;
        const RegistroPartida *q = &quedas[GetRandomValue(ini, fim)];

        /* nasce longe do mago, como os outros, mas é um caçador reforçado */
        int antes = p->ecos.total;
        spawnar_ecos_extras(&p->ecos, 1, p->jogo.andar, &p->andar.mapa, &p->mago);
        if (p->ecos.total > antes) {
            Eco *e = &p->ecos.itens[p->ecos.total - 1];
            e->tipo      = ECO_CACADOR;
            e->lembranca = 1;
            e->vida_max *= 2;
            e->vida      = e->vida_max;
            e->raio      = 18.0f;
            e->velocidade *= 0.9f;
            snprintf(e->nome, sizeof(e->nome), "Eco de %s", q->nome);

            char data[16] = "?";
            time_t quando = (time_t)q->data;
            struct tm *t = localtime(&quando);
            if (t != NULL) strftime(data, sizeof(data), "%d/%m", t);

            if (comparar_nomes(q->nome, p->jogo.jogador) == 0) {
                registrar_evento(p, "Seu proprio eco te espera: voce caiu aqui em %s.", data);
            } else {
                registrar_evento(p, "Um eco familiar: %s caiu aqui em %s.", q->nome, data);
            }
        }
    }
    free(quedas);
}

void liberar_partida(Partida *p) {
    if (p == NULL) return;
    liberar_equipe_ecos(&p->ecos);
}

void escolher_melhoria(Partida *p, int indice) {
    if (p == NULL) return;
    Jogo *j = &p->jogo;
    if (j->estado != ESTADO_MELHORIA) return;
    if (indice < 0 || indice >= OFERTAS_MELHORIA) return;

    aplicar_melhoria(&p->mago, j->ofertas[indice]);

    /* subir a escada dá um fôlego */
    p->mago.vida += CURA_ESCADA;
    if (p->mago.vida > p->mago.vida_max) p->mago.vida = p->mago.vida_max;

    entrar_no_andar(p, j->andar + 1);
    j->estado = ESTADO_JOGANDO;
}

void recarregar_andar(Partida *p) {
    if (p == NULL || p->jogo.estado != ESTADO_JOGANDO) return;
    entrar_no_andar(p, p->jogo.andar);
}

void atualizar_partida(Partida *p, Controles c, float dt) {
    if (p == NULL) return;
    Jogo *j = &p->jogo;
    if (j->estado != ESTADO_JOGANDO) return;
    const Mapa *mapa = &p->andar.mapa;

    j->tempo_partida += dt;
    reduzir_tempo(&j->tempo_mensagem, dt);
    reduzir_tempo(&j->tempo_titulo, dt);
    reduzir_tempo(&p->tremor, dt);
    p->desempenho.testes_separacao = 0;

    /* 1) jogador, 2) mapa de distâncias, 3) ecos, 4) projéteis, 5) efeitos */
    atualizar_mago(&p->mago, c, mapa, &p->projeteis, dt);

    calcular_distancias(mapa, p->mago.pos, &p->distancias);

    int dano = atualizar_ecos(&p->ecos, &p->mago, mapa, &p->distancias,
                              &p->projeteis, &p->efeitos, &p->desempenho, dt);

    int destruidos = 0, lembrancas = 0;
    dano += atualizar_projeteis(&p->projeteis, mapa, &p->mago, &p->ecos,
                                &p->efeitos, dt, &destruidos, &lembrancas);

    atualizar_efeitos(&p->efeitos, dt);

    if (destruidos > 0) {
        j->ecos_derrotados += destruidos;
        if (lembrancas > 0) {
            j->bonus += lembrancas * BONUS_LEMBRANCA;
            registrar_evento(p, "O eco lembranca descansou. +%d pontos!", lembrancas * BONUS_LEMBRANCA);
        } else {
            registrar_evento(p, "Eco destruido! Restam %d.", p->ecos.total);
        }
    }
    if (dano > 0) {
        registrar_evento(p, "Voce levou %d de dano. Vida: %d/%d.",
                         dano, p->mago.vida, p->mago.vida_max);
        p->tremor = TEMPO_TREMOR;
    }

    /* 6) derrota, escada, troca de onda */
    if (p->mago.vida <= 0) {
        finalizar_partida(p, 0);

    } else if (j->escada_aberta) {
        if (vdistancia(p->mago.pos, posicao_escada(p)) < RAIO_ESCADA) {
            if (j->andar >= j->andar_max) {
                finalizar_partida(p, 1);
            } else {
                sortear_melhorias(j->ofertas, &p->mago);
                j->estado = ESTADO_MELHORIA;
            }
        }

    } else if (p->ecos.total == 0) {
        if (j->intervalo > 0.0f) {
            j->intervalo -= dt;
            if (j->intervalo <= 0.0f) {
                j->intervalo = 0.0f;
                j->onda++;
                spawnar_onda(&p->ecos, j->andar, j->onda, mapa, &p->mago);
                registrar_evento(p, "Onda %d de %d! Ecos: %d.",
                                 j->onda, j->ondas_no_andar, p->ecos.total);
                if (j->onda == 1) spawnar_lembranca(p);   /* aparece junto da 1a onda */
            }
        } else if (j->onda >= j->ondas_no_andar) {
            j->escada_aberta = 1;
            if (j->andar >= j->andar_max) registrar_evento(p, "A saida da torre se abriu! Va ate a escada.");
            else                          registrar_evento(p, "Andar limpo! Va ate a escada.");
        } else {
            j->intervalo = TEMPO_INTERVALO;
            registrar_evento(p, "Onda %d de %d limpa!", j->onda, j->ondas_no_andar);
        }
    }

    /* 7) por último, deixa os ecos ordenados por Y para o desenho
          (inclusive os que acabaram de nascer numa onda nova) */
    p->desempenho.comparacoes_ordenacao =
        insertion_sort_ecos_por_y(p->ecos.itens, p->ecos.total);
}

/* ======================== desenho ======================== */

static void desenhar_escada(const Partida *p) {
    Vector2 c = posicao_escada(p);
    int x = (int)(c.x - TAM_TILE / 2.0f);
    int y = (int)(c.y - TAM_TILE / 2.0f);
    int lado = TAM_TILE - 12;

    if (!p->jogo.escada_aberta) {
        /* alçapão fechado */
        DrawRectangle(x + 6, y + 6, lado, lado, (Color){ 38, 32, 50, 255 });
        DrawRectangleLines(x + 6, y + 6, lado, lado, (Color){ 72, 64, 92, 255 });
        DrawRectangle(x + 10, y + 16, lado - 8, 3, (Color){ 60, 52, 78, 255 });
        DrawRectangle(x + 10, y + 28, lado - 8, 3, (Color){ 60, 52, 78, 255 });
        return;
    }

    /* escada aberta: brilha pulsando para chamar atenção */
    float pulso = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
    DrawCircleV(c, TAM_TILE * (0.6f + 0.15f * pulso),
                Fade((Color){ 255, 220, 120, 255 }, 0.15f + 0.15f * pulso));
    DrawRectangle(x + 6, y + 6, lado, lado, (Color){ 70, 52, 30, 255 });
    for (int k = 0; k < 4; k++) {   /* degraus */
        DrawRectangle(x + 10 + k * 3, y + 10 + k * 8, lado - 8 - k * 6, 5,
                      (Color){ 230, 190, 110, 255 });
    }
}

/* Setinha perto do mago apontando para a escada aberta. */
static void desenhar_seta_escada(const Partida *p) {
    Vector2 alvo = posicao_escada(p);
    if (vdistancia(alvo, p->mago.pos) < 90.0f) return;

    Vector2 dir   = vnormaliza(vsub(alvo, p->mago.pos));
    Vector2 ponta = vsoma(p->mago.pos, vescala(dir, 48.0f));
    Vector2 base  = vsoma(p->mago.pos, vescala(dir, 38.0f));
    Vector2 lado  = { -dir.y * 8.0f, dir.x * 8.0f };
    Color   cor   = { 255, 220, 120, 230 };
    DrawLineEx(ponta, vsoma(base, lado), 3.0f, cor);
    DrawLineEx(ponta, vsub(base, lado),  3.0f, cor);
}

void desenhar_mundo(const Partida *p, const Assets *a) {
    if (p == NULL || a == NULL) return;

    desenhar_mapa(&p->andar.mapa);
    desenhar_escada(p);

    if (p->jogo.debug) {   /* mostra o mapa de distâncias que os ecos seguem */
        char num[12];
        for (int y = 0; y < MAPA_ALTURA; y++) {
            for (int x = 0; x < MAPA_LARGURA; x++) {
                if (p->distancias.d[y][x] < 0) continue;
                snprintf(num, sizeof(num), "%d", p->distancias.d[y][x]);
                DrawText(num, x * TAM_TILE + 4, y * TAM_TILE + 4, 10, Fade(SKYBLUE, 0.45f));
            }
        }
    }

    /*
     * Os ecos já estão ordenados por Y (Insertion Sort). O mago entra
     * "intercalado" na posição certa: é desenhado logo antes do primeiro
     * eco que estiver mais embaixo do que ele.
     */
    int mago_desenhado = 0;
    for (int i = 0; i < p->ecos.total; i++) {
        const Eco *e = &p->ecos.itens[i];
        if (!mago_desenhado && p->mago.pos.y < e->pos.y) {
            desenhar_personagem(&p->mago, a);
            mago_desenhado = 1;
        }
        desenhar_eco(e, a);
    }
    if (!mago_desenhado) desenhar_personagem(&p->mago, a);

    desenhar_projeteis(&p->projeteis);
    desenhar_efeitos(&p->efeitos);

    if (p->jogo.escada_aberta) desenhar_seta_escada(p);

    if (p->jogo.debug) {   /* mostra os círculos de colisão */
        DrawCircleLines((int)p->mago.pos.x, (int)p->mago.pos.y, p->mago.raio, GREEN);
        for (int i = 0; i < p->ecos.total; i++) {
            const Eco *e = &p->ecos.itens[i];
            DrawCircleLines((int)e->pos.x, (int)e->pos.y, e->raio, RED);
        }
    }
}
