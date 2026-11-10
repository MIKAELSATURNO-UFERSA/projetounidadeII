#include "mapa.h"
#include "vetor.h"
#include <raylib.h>
#include <math.h>

int eh_parede(const Mapa *m, int tx, int ty) {
    if (tx < 0 || tx >= MAPA_LARGURA) return 1;
    if (ty < 0 || ty >= MAPA_ALTURA)  return 1;
    return m->tiles[ty][tx] == TILE_PAREDE;
}

Vector2 centro_do_tile(int tx, int ty) {
    return (Vector2){ tx * TAM_TILE + TAM_TILE / 2.0f,
                      ty * TAM_TILE + TAM_TILE / 2.0f };
}

int circulo_colide_parede(const Mapa *m, Vector2 c, float r) {
    /* Só olha os tiles que o "quadrado em volta" do círculo toca. */
    int x0 = (int)floorf((c.x - r) / TAM_TILE);
    int x1 = (int)floorf((c.x + r) / TAM_TILE);
    int y0 = (int)floorf((c.y - r) / TAM_TILE);
    int y1 = (int)floorf((c.y + r) / TAM_TILE);

    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            if (!eh_parede(m, tx, ty)) continue;

            /* Ponto do tile mais perto do centro do círculo. */
            float px = fmaxf(tx * TAM_TILE, fminf(c.x, (tx + 1) * TAM_TILE));
            float py = fmaxf(ty * TAM_TILE, fminf(c.y, (ty + 1) * TAM_TILE));
            float dx = c.x - px;
            float dy = c.y - py;
            if (dx * dx + dy * dy < r * r) return 1;
        }
    }
    return 0;
}

int mover_com_colisao(const Mapa *m, Vector2 *pos, Vector2 desloc, float raio) {
    /*
     * Anda em passinhos de no máximo 4 px. Assim, mesmo rápido (na esquiva),
     * o personagem para encostado na parede em vez de parar longe dela.
     */
    float maior = fmaxf(fabsf(desloc.x), fabsf(desloc.y));
    int passos = (int)ceilf(maior / 4.0f);
    if (passos < 1) passos = 1;

    Vector2 passo = { desloc.x / passos, desloc.y / passos };
    int bateu = 0;

    for (int i = 0; i < passos; i++) {
        Vector2 tenta = { pos->x + passo.x, pos->y };
        if (!circulo_colide_parede(m, tenta, raio)) pos->x = tenta.x;
        else { passo.x = 0.0f; bateu = 1; }

        tenta = (Vector2){ pos->x, pos->y + passo.y };
        if (!circulo_colide_parede(m, tenta, raio)) pos->y = tenta.y;
        else { passo.y = 0.0f; bateu = 1; }
    }
    return bateu;
}

int linha_de_visao(const Mapa *m, Vector2 a, Vector2 b) {
    float dist = vdistancia(a, b);
    int amostras = (int)(dist / 8.0f) + 1;    /* um ponto a cada 8 px */

    for (int i = 0; i <= amostras; i++) {
        float t = (float)i / amostras;
        float x = a.x + (b.x - a.x) * t;
        float y = a.y + (b.y - a.y) * t;
        if (eh_parede(m, (int)floorf(x / TAM_TILE), (int)floorf(y / TAM_TILE))) {
            return 0;
        }
    }
    return 1;
}

int caminho_livre(const Mapa *m, Vector2 a, Vector2 b, float raio) {
    /* testa a linha do centro e duas linhas paralelas nas "bordas" do círculo */
    Vector2 dir  = vnormaliza(vsub(b, a));
    Vector2 lado = vescala((Vector2){ -dir.y, dir.x }, raio);
    return linha_de_visao(m, a, b) &&
           linha_de_visao(m, vsoma(a, lado), vsoma(b, lado)) &&
           linha_de_visao(m, vsub(a, lado),  vsub(b, lado));
}

void calcular_distancias(const Mapa *m, Vector2 alvo, MapaDistancias *md) {
    for (int y = 0; y < MAPA_ALTURA; y++)
        for (int x = 0; x < MAPA_LARGURA; x++)
            md->d[y][x] = -1;

    int ax = (int)floorf(alvo.x / TAM_TILE);
    int ay = (int)floorf(alvo.y / TAM_TILE);
    if (eh_parede(m, ax, ay)) return;

    /*
     * Busca em largura (BFS) com uma fila: começa no tile do alvo (distância 0),
     * depois os vizinhos (1), os vizinhos dos vizinhos (2)... Cada tile entra
     * na fila uma vez só, então custa O(número de tiles) = 260 passos.
     */
    int fila_x[MAPA_LARGURA * MAPA_ALTURA];
    int fila_y[MAPA_LARGURA * MAPA_ALTURA];
    int inicio = 0, fim = 0;

    md->d[ay][ax] = 0;
    fila_x[fim] = ax; fila_y[fim] = ay; fim++;

    const int vx[4] = { 1, -1, 0,  0 };
    const int vy[4] = { 0,  0, 1, -1 };

    while (inicio < fim) {
        int x = fila_x[inicio], y = fila_y[inicio];
        inicio++;
        for (int k = 0; k < 4; k++) {
            int nx = x + vx[k], ny = y + vy[k];
            if (eh_parede(m, nx, ny) || md->d[ny][nx] != -1) continue;
            md->d[ny][nx] = md->d[y][x] + 1;
            fila_x[fim] = nx; fila_y[fim] = ny; fim++;
        }
    }
}

Vector2 direcao_pelo_mapa(const Mapa *m, const MapaDistancias *md, Vector2 pos) {
    int tx = (int)floorf(pos.x / TAM_TILE);
    int ty = (int)floorf(pos.y / TAM_TILE);
    if (eh_parede(m, tx, ty)) return (Vector2){ 0.0f, 0.0f };

    int atual = md->d[ty][tx];
    if (atual <= 0) return (Vector2){ 0.0f, 0.0f };

    int melhor = atual, mx = tx, my = ty;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = tx + dx, ny = ty + dy;
            if (eh_parede(m, nx, ny)) continue;
            /* na diagonal, só se não for "cortar" a quina de uma parede */
            if (dx != 0 && dy != 0 &&
                (eh_parede(m, tx + dx, ty) || eh_parede(m, tx, ty + dy))) continue;
            int d = md->d[ny][nx];
            if (d >= 0 && d < melhor) { melhor = d; mx = nx; my = ny; }
        }
    }
    if (mx == tx && my == ty) return (Vector2){ 0.0f, 0.0f };
    return vnormaliza(vsub(centro_do_tile(mx, my), pos));
}

void desenhar_mapa(const Mapa *m) {
    for (int y = 0; y < MAPA_ALTURA; y++) {
        for (int x = 0; x < MAPA_LARGURA; x++) {
            int px = x * TAM_TILE;
            int py = y * TAM_TILE;

            if (m->tiles[y][x] == TILE_PAREDE) {
                DrawRectangle(px, py, TAM_TILE, TAM_TILE, (Color){ 46, 44, 68, 255 });
                /* "topo" mais claro dá um pouco de volume à parede */
                DrawRectangle(px, py, TAM_TILE, 8, (Color){ 70, 66, 100, 255 });
                DrawRectangleLines(px, py, TAM_TILE, TAM_TILE, (Color){ 30, 28, 45, 255 });
            } else {
                Color chao = ((x + y) % 2 == 0) ? (Color){ 26, 22, 36, 255 }
                                                : (Color){ 29, 25, 40, 255 };
                DrawRectangle(px, py, TAM_TILE, TAM_TILE, chao);
            }
        }
    }
}
