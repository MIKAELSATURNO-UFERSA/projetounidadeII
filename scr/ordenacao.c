#include "ordenacao.h"
#include <raylib.h>
#include <stdlib.h>
#include <ctype.h>

/* ======================= Insertion Sort ======================= */

/*
 * Por que ordenar por Y?
 * A câmera vê a sala "de cima e de frente". Quem está mais embaixo na tela
 * está mais perto de quem olha, então precisa ser desenhado POR ÚLTIMO
 * (por cima dos outros). Se desenhar na ordem errada, um eco que está
 * na frente do mago aparece atrás dele.
 *
 * Por que Insertion Sort?
 * A ordenação roda a cada quadro (60 vezes por segundo). De um quadro para o
 * outro cada eco anda só uns 2 pixels, então o vetor chega QUASE ORDENADO.
 * Nesse caso o laço de dentro (while) quase nunca anda: o Insertion Sort faz
 * perto de n-1 comparações, ou seja, O(n) — o melhor caso dele.
 * Ligue o modo debug (F1) no jogo para ver o número de comparações.
 */
int insertion_sort_ecos_por_y(Eco *v, int n) {
    int comparacoes = 0;

    for (int i = 1; i < n; i++) {
        Eco chave = v[i];
        int j = i - 1;

        /* empurra para a direita quem tem Y maior que a chave */
        while (j >= 0) {
            comparacoes++;
            if (v[j].pos.y <= chave.pos.y) break;
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = chave;
    }
    return comparacoes;
}

/* ======================= comparação de registros ======================= */

int comparar_nomes(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* Ajudantes para comparar números sem risco de estouro (a - b pode estourar). */
static int menor_primeiro(long long a, long long b) { return (a < b) ? -1 : (a > b); }
static int maior_primeiro(long long a, long long b) { return (a > b) ? -1 : (a < b); }

int comparar_registros(const RegistroPartida *a, const RegistroPartida *b, Criterio c) {
    int r = 0;
    /* Cada critério tem desempates, para a ordem ficar sempre bem definida. */
    switch (c) {
    case CRITERIO_PONTOS:
        r = maior_primeiro(a->pontos, b->pontos);
        if (r == 0) r = menor_primeiro(a->tempo, b->tempo);
        break;
    case CRITERIO_ANDAR:
        r = maior_primeiro(a->andar, b->andar);
        if (r == 0) r = maior_primeiro(a->venceu, b->venceu);
        if (r == 0) r = menor_primeiro(a->tempo, b->tempo);
        break;
    case CRITERIO_NOME:
        r = comparar_nomes(a->nome, b->nome);
        if (r == 0) r = maior_primeiro(a->pontos, b->pontos);
        break;
    case CRITERIO_DATA:
    default:
        break;
    }
    if (r == 0) r = maior_primeiro(a->data, b->data);   /* último desempate: mais recente */
    return r;
}

const char *nome_criterio(Criterio c) {
    switch (c) {
    case CRITERIO_PONTOS: return "PONTOS";
    case CRITERIO_ANDAR:  return "ANDAR";
    case CRITERIO_NOME:   return "NOME";
    case CRITERIO_DATA:   return "RECENTES";
    default:              return "?";
    }
}

const char *nome_algoritmo(AlgoritmoOrdenacao a) {
    return (a == ALGORITMO_QUICK) ? "Quick Sort" : "Merge Sort";
}

static void trocar(RegistroPartida *a, RegistroPartida *b) {
    RegistroPartida t = *a;
    *a = *b;
    *b = t;
}

/* ======================= Merge Sort ======================= */

/*
 * Intercalação: junta duas metades JÁ ORDENADAS, v[ini..meio-1] e v[meio..fim-1],
 * numa só, usando o vetor auxiliar. Em cada passo compara o primeiro de cada
 * metade e copia o menor. Custo: O(n) para n elementos.
 */
static void intercalar(RegistroPartida *v, RegistroPartida *aux, int ini, int meio, int fim,
                       Criterio c, long *comparacoes) {
    int i = ini, j = meio, k = ini;

    while (i < meio && j < fim) {
        if (comparacoes != NULL) (*comparacoes)++;
        /* só pega o da direita se ele for ESTRITAMENTE menor: isso deixa o
           Merge Sort estável (empates mantêm a ordem em que estavam) */
        if (comparar_registros(&v[j], &v[i], c) < 0) aux[k++] = v[j++];
        else                                         aux[k++] = v[i++];
    }
    while (i < meio) aux[k++] = v[i++];   /* sobrou da esquerda */
    while (j < fim)  aux[k++] = v[j++];   /* sobrou da direita */

    for (k = ini; k < fim; k++) v[k] = aux[k];
}

/*
 * Divisão e conquista: divide no meio, ordena cada metade (recursão) e intercala.
 * São log2(n) níveis de divisão, cada nível intercala n elementos: O(n log n).
 */
static void merge_sort_rec(RegistroPartida *v, RegistroPartida *aux, int ini, int fim,
                           Criterio c, long *comparacoes) {
    if (fim - ini < 2) return;            /* caso base: 0 ou 1 elemento já está ordenado */

    int meio = ini + (fim - ini) / 2;
    merge_sort_rec(v, aux, ini, meio, c, comparacoes);
    merge_sort_rec(v, aux, meio, fim, c, comparacoes);
    intercalar(v, aux, ini, meio, fim, c, comparacoes);
}

void merge_sort_registros(RegistroPartida *v, int n, Criterio c, long *comparacoes) {
    if (v == NULL || n < 2) return;

    RegistroPartida *aux = (RegistroPartida *) malloc(n * sizeof(RegistroPartida));
    if (aux == NULL) {
        TraceLog(LOG_WARNING, "Sem memoria para o Merge Sort; usando Quick Sort.");
        quick_sort_registros(v, n, c, comparacoes);
        return;
    }
    merge_sort_rec(v, aux, 0, n, c, comparacoes);
    free(aux);
}

/* ======================= Quick Sort ======================= */

/*
 * Partição: escolhe um pivô e reorganiza v[ini..fim] para que fiquem
 * à esquerda dele os que vêm antes e à direita os outros.
 * Retorna a posição final do pivô (que já está no lugar certo).
 *
 * O pivô é SORTEADO: se fosse sempre o último elemento, um vetor que já
 * chega ordenado faria o Quick Sort cair no pior caso, O(n²).
 */
static int particionar(RegistroPartida *v, int ini, int fim, Criterio c, long *comparacoes) {
    int sorteado = GetRandomValue(ini, fim);
    trocar(&v[sorteado], &v[fim]);           /* o pivô sorteado vai para o fim */
    RegistroPartida pivo = v[fim];

    int i = ini;                             /* próxima vaga do lado "antes do pivô" */
    for (int j = ini; j < fim; j++) {
        if (comparacoes != NULL) (*comparacoes)++;
        if (comparar_registros(&v[j], &pivo, c) < 0) {
            trocar(&v[i], &v[j]);
            i++;
        }
    }
    trocar(&v[i], &v[fim]);                  /* pivô entre os dois lados */
    return i;
}

static void quick_sort_rec(RegistroPartida *v, int ini, int fim, Criterio c, long *comparacoes) {
    if (ini >= fim) return;                  /* caso base: 0 ou 1 elemento */

    int p = particionar(v, ini, fim, c, comparacoes);
    quick_sort_rec(v, ini, p - 1, c, comparacoes);
    quick_sort_rec(v, p + 1, fim, c, comparacoes);
}

void quick_sort_registros(RegistroPartida *v, int n, Criterio c, long *comparacoes) {
    if (v == NULL || n < 2) return;
    quick_sort_rec(v, 0, n - 1, c, comparacoes);
}

void ordenar_registros(RegistroPartida *v, int n, Criterio c,
                       AlgoritmoOrdenacao alg, long *comparacoes) {
    if (alg == ALGORITMO_QUICK) quick_sort_registros(v, n, c, comparacoes);
    else                        merge_sort_registros(v, n, c, comparacoes);
}
