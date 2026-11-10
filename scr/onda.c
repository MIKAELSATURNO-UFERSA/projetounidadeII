#include "onda.h"
#include "mapa.h"
#include "equipe.h"
#include "vetor.h"
#include <raylib.h>

#define DISTANCIA_MINIMA_DO_MAGO 220.0f   /* ninguém nasce colado no jogador */
#define RAIO_PARA_SPAWN           18.0f

/*
 * Andar 1: ondas com 3 e 4 ecos ... andar 5: ondas com 5, 6 e 7 ecos.
 * (A cada 2 andares entra 1 eco a mais por onda.)
 * Os atiradores aparecem a partir da 2a onda do primeiro andar.
 */
int ecos_para_onda(int andar, int onda) {
    if (andar < 1) andar = 1;
    if (onda < 1)  onda = 1;
    return 2 + andar / 2 + onda;
}

int atiradores_na_onda(int andar, int onda) {
    if (andar < 1) andar = 1;
    if (onda < 1)  onda = 1;
    return (andar + onda - 1) / 2;
}

/*
 * Sorteia o centro de um tile de chão que não esteja ocupado nem perto
 * do mago. Usa GetRandomValue (da raylib), que respeita o SetRandomSeed
 * do main — o rand() de antes sempre sorteava as mesmas posições.
 */
static Vector2 posicao_livre(const Mapa *m, const EquipeEcos *eq,
                             const Personagem *mago) {
    Vector2 p = centro_do_tile(1, 1);

    for (int tentativa = 0; tentativa < 400; tentativa++) {
        int tx = GetRandomValue(1, MAPA_LARGURA - 2);
        int ty = GetRandomValue(1, MAPA_ALTURA  - 2);
        p = centro_do_tile(tx, ty);

        if (eh_parede(m, tx, ty)) continue;
        if (circulo_colide_parede(m, p, RAIO_PARA_SPAWN)) continue;
        if (eco_perto(eq, p, 40.0f)) continue;

        /* depois de muitas tentativas, aceita ficar um pouco mais perto */
        float minimo = (tentativa < 300) ? DISTANCIA_MINIMA_DO_MAGO : 100.0f;
        if (vdistancia(p, mago->pos) < minimo) continue;
        return p;
    }

    /* Plano B: o primeiro tile de chão livre, varrendo o mapa. */
    for (int ty = 1; ty < MAPA_ALTURA - 1; ty++) {
        for (int tx = 1; tx < MAPA_LARGURA - 1; tx++) {
            Vector2 c = centro_do_tile(tx, ty);
            if (!eh_parede(m, tx, ty) && !circulo_colide_parede(m, c, RAIO_PARA_SPAWN) &&
                vdistancia(c, mago->pos) > 60.0f) {
                return c;
            }
        }
    }
    return p;
}

void spawnar_onda(EquipeEcos *ecos, int andar, int onda, const Mapa *m, const Personagem *mago) {
    if (ecos == NULL || m == NULL || mago == NULL) return;

    int quantidade = ecos_para_onda(andar, onda);
    int atiradores = atiradores_na_onda(andar, onda);

    for (int i = 0; i < quantidade; i++) {
        TipoEco tipo = (i < atiradores) ? ECO_ATIRADOR : ECO_CACADOR;
        Vector2 p = posicao_livre(m, ecos, mago);
        adicionar_eco(ecos, criar_eco(andar, tipo, p));
    }
}

void spawnar_ecos_extras(EquipeEcos *ecos, int quantidade, int andar,
                         const Mapa *m, const Personagem *mago) {
    if (ecos == NULL || m == NULL || mago == NULL) return;
    for (int i = 0; i < quantidade; i++) {
        TipoEco tipo = (i % 4 == 0) ? ECO_ATIRADOR : ECO_CACADOR;
        adicionar_eco(ecos, criar_eco(andar, tipo, posicao_livre(m, ecos, mago)));
    }
}
