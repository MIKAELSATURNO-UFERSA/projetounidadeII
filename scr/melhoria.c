#include "melhoria.h"
#include <raylib.h>

static const char *NOMES[TOTAL_MELHORIAS] = {
    "Vigor",
    "Descanso",
    "Esfera Afiada",
    "Maos Rapidas",
    "Passos Leves",
    "Reflexos",
};

static const char *DESCRICOES[TOTAL_MELHORIAS] = {
    "+5 de vida maxima\n(e recupera 5)",
    "Recupera toda\na vida",
    "+2 de dano\nem cada esfera",
    "Esferas recarregam\n15% mais rapido",
    "Anda 10% mais\nrapido",
    "Esquiva recarrega\n25% mais rapido",
};

const char *nome_melhoria(TipoMelhoria t) {
    if ((int)t < 0 || t >= TOTAL_MELHORIAS) return "?";
    return NOMES[t];
}

const char *descricao_melhoria(TipoMelhoria t) {
    if ((int)t < 0 || t >= TOTAL_MELHORIAS) return "";
    return DESCRICOES[t];
}

void sortear_melhorias(TipoMelhoria saida[OFERTAS_MELHORIA], const Personagem *mago) {
    /* Monta a lista de candidatas... */
    TipoMelhoria candidatas[TOTAL_MELHORIAS];
    int n = 0;
    for (int t = 0; t < TOTAL_MELHORIAS; t++) {
        if (t == MELHORIA_CURA && mago != NULL && mago->vida >= mago->vida_max) continue;
        candidatas[n++] = (TipoMelhoria)t;
    }

    /* ...e embaralha só o começo dela (Fisher-Yates): cada sorteio troca a
       posição i com uma posição aleatória entre i e o fim. */
    for (int i = 0; i < OFERTAS_MELHORIA && i < n; i++) {
        int j = GetRandomValue(i, n - 1);
        TipoMelhoria tmp = candidatas[i];
        candidatas[i] = candidatas[j];
        candidatas[j] = tmp;
        saida[i] = candidatas[i];
    }
}

void aplicar_melhoria(Personagem *mago, TipoMelhoria t) {
    if (mago == NULL) return;

    switch (t) {
    case MELHORIA_VIDA:
        mago->vida_max += 5;
        mago->vida     += 5;
        break;
    case MELHORIA_CURA:
        mago->vida = mago->vida_max;
        break;
    case MELHORIA_DANO:
        mago->ataque += 2;
        break;
    case MELHORIA_RECARGA:
        mago->tempo_entre_esferas *= 0.85f;
        if (mago->tempo_entre_esferas < 0.12f) mago->tempo_entre_esferas = 0.12f;
        break;
    case MELHORIA_VELOCIDADE:
        mago->velocidade *= 1.10f;
        if (mago->velocidade > 290.0f) mago->velocidade = 290.0f;
        break;
    case MELHORIA_ESQUIVA:
        mago->tempo_recarga_esquiva *= 0.75f;
        if (mago->tempo_recarga_esquiva < 0.35f) mago->tempo_recarga_esquiva = 0.35f;
        break;
    default:
        break;
    }
    if (mago->vida > mago->vida_max) mago->vida = mago->vida_max;
}
