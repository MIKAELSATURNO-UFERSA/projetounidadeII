#ifndef VETOR_H
#define VETOR_H

/*
 * Contas pequenas com vetores 2D (Vector2 da raylib).
 * São "static inline": cada .c que inclui ganha uma cópia, sem precisar de .c próprio.
 */

#include <raylib.h>
#include <math.h>

static inline Vector2 vsoma(Vector2 a, Vector2 b)    { return (Vector2){ a.x + b.x, a.y + b.y }; }
static inline Vector2 vsub(Vector2 a, Vector2 b)     { return (Vector2){ a.x - b.x, a.y - b.y }; }
static inline Vector2 vescala(Vector2 a, float k)    { return (Vector2){ a.x * k, a.y * k }; }
static inline float   vtamanho(Vector2 a)            { return sqrtf(a.x * a.x + a.y * a.y); }
static inline float   vdistancia(Vector2 a, Vector2 b) { return vtamanho(vsub(a, b)); }

/* Mesmo sentido, tamanho 1. Vetor zero continua zero. */
static inline Vector2 vnormaliza(Vector2 a) {
    float t = vtamanho(a);
    if (t < 0.0001f) return (Vector2){ 0.0f, 0.0f };
    return vescala(a, 1.0f / t);
}

/* Diminui um temporizador sem deixar ficar negativo. */
static inline void reduzir_tempo(float *t, float dt) {
    *t -= dt;
    if (*t < 0.0f) *t = 0.0f;
}

/*
 * Faz o empurrão "morrer" aos poucos (atrito).
 * Quanto maior a força, mais rápido ele some.
 */
static inline Vector2 amortecer(Vector2 v, float forca, float dt) {
    float k = 1.0f - forca * dt;
    if (k < 0.0f) k = 0.0f;
    return vescala(v, k);
}

#endif
