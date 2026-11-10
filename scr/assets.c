#include "assets.h"
#include <raylib.h>
#include <stddef.h>

int carregar_assets(Assets *a) {
    if (a == NULL) return 0;

    a->mago = LoadTexture("assets/sprites/mago.png");
    a->eco  = LoadTexture("assets/sprites/eco.png");

    if (a->mago.id == 0 || a->eco.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar sprites.");
        return 0;
    }
    return 1;
}

void liberar_assets(Assets *a) {
    if (a == NULL) return;
    UnloadTexture(a->mago);
    UnloadTexture(a->eco);
}