#ifndef ASSETS_H
#define ASSETS_H

#include <raylib.h>

typedef struct {
    Texture2D mago;
    Texture2D eco;
} Assets;

int  carregar_assets(Assets *a);
void liberar_assets(Assets *a);

#endif