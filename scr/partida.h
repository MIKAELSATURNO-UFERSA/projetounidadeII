#ifndef PARTIDA_H
#define PARTIDA_H

/*
 * partida.c — as regras do jogo, quadro a quadro.
 * O main.c cuida da janela, das teclas de menu e da ordem de desenho;
 * aqui fica "o que acontece no mundo" a cada 1/60 de segundo.
 */

#include "jogo.h"
#include "personagem.h"
#include "assets.h"
#include "diario.h"

typedef struct {
    Jogo                 jogo;
    Andar                andar;        /* andar atual: nome, ondas, mapa, início e escada */
    char                 aviso_andar[200];  /* erro ao ler o arquivo do andar ("" = tudo certo) */
    MapaDistancias       distancias;   /* passos de cada tile até o mago */
    Personagem           mago;
    EquipeEcos           ecos;
    GerenciadorProjeteis projeteis;
    GerenciadorEfeitos   efeitos;
    Desempenho           desempenho;
    float                tremor;       /* tempo restante de tela tremendo */

    Historico           *historico;    /* todas as partidas (o vetor é do main) */
    char                 resumo[MAX_LINHAS_RESUMO][TAM_LINHA_DIARIO];  /* fim do diário */
    int                  qtd_resumo;
} Partida;

/* Uma vez, quando o programa abre. O histórico é usado para os ecos lembrança
   e recebe o registro de cada partida que termina. */
void preparar_partida(Partida *p, Historico *h);

/* Toda vez que uma partida nova começa. */
void reiniciar_partida(Partida *p, const char *jogador);

void liberar_partida(Partida *p);    /* quando o programa fecha */

void atualizar_partida(Partida *p, Controles c, float dt);

/* Na tela de melhorias: aplica a melhoria escolhida (0, 1 ou 2) e sobe um andar. */
void escolher_melhoria(Partida *p, int indice);

/* Lê de novo o arquivo do andar atual (F3 no modo debug), para testar edições no .txt. */
void recarregar_andar(Partida *p);

/* Centro da escada do andar atual, em pixels. */
Vector2 posicao_escada(const Partida *p);

/* Desenha a sala e tudo dentro dela (em coordenadas da sala). */
void desenhar_mundo(const Partida *p, const Assets *a);

/* Guarda a mensagem que aparece no rodapé (funciona como printf). */
void registrar_evento(Partida *p, const char *formato, ...);

#endif
