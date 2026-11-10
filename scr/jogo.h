#ifndef JOGO_H
#define JOGO_H

/*
 * jogo.h — todas as estruturas de dados do jogo ficam aqui.
 *
 * Os outros .h só declaram funções. Assim nenhum módulo precisa incluir
 * o outro "em círculo" (ecos precisam de projéteis, projéteis precisam
 * de ecos...), e quem quiser entender os dados do jogo olha um arquivo só.
 */

#include <raylib.h>
#include <stddef.h>

/* ---------- Tela e sala ---------- */

#define TAM_TILE      48          /* sprites 16x16 desenhados em escala 3x */
#define MAPA_LARGURA  20
#define MAPA_ALTURA   13
#define ALTURA_HUD    64
#define ALTURA_RODAPE 40

#define LARGURA_SALA  (MAPA_LARGURA * TAM_TILE)   /* 960 px */
#define ALTURA_SALA   (MAPA_ALTURA  * TAM_TILE)   /* 624 px */

#define LARGURA_TELA  LARGURA_SALA
#define ALTURA_TELA   (ALTURA_HUD + ALTURA_SALA + ALTURA_RODAPE)  /* 728 px */

/* ---------- Limites dos vetores de tamanho fixo ---------- */

#define MAX_PROJETEIS   128
#define MAX_MENSAGENS   32
#define MAX_IMPACTOS    48
#define TAM_NOME        16     /* nome do jogador, com o '\0' */

/* ---------- Mapa ---------- */

typedef enum {
    TILE_CHAO = 0,
    TILE_PAREDE
} TipoTile;

typedef struct {
    TipoTile tiles[MAPA_ALTURA][MAPA_LARGURA];
} Mapa;

/*
 * Para cada tile, quantos passos faltam até o mago (-1 = parede ou sem caminho).
 * Recalculado a cada quadro; os ecos seguem os números diminuindo para
 * contornar paredes em vez de ficarem presos nelas.
 */
typedef struct {
    int d[MAPA_ALTURA][MAPA_LARGURA];
} MapaDistancias;

/* Um andar da torre, lido de assets/andares/andarN.txt */
typedef struct {
    char nome[32];
    int  ondas;                 /* quantas ondas até a escada abrir */
    Mapa mapa;
    int  inicio_x, inicio_y;    /* tile 'M' do arquivo: onde o mago começa */
    int  escada_x, escada_y;    /* tile 'S' do arquivo: onde fica a escada */
} Andar;

/* ---------- Estados da tela ---------- */

typedef enum {
    ESTADO_MENU = 0,
    ESTADO_JOGANDO,
    ESTADO_PAUSADO,
    ESTADO_MELHORIA,     /* entre andares: escolhendo 1 de 3 melhorias */
    ESTADO_NOME,         /* digitando o nome antes de começar */
    ESTADO_RANKING,      /* tela de ranking (aberta pelo menu) */
    ESTADO_VITORIA,
    ESTADO_DERROTA
} EstadoJogo;

/*
 * Posições agora são em PIXELS (float), não em quadradinhos.
 * Vector2 vem da raylib: struct { float x; float y; }.
 * Todas as posições são relativas à sala (o canto de cima da sala é y = 0);
 * o deslocamento do HUD é aplicado só na hora de desenhar, pela câmera.
 */

typedef struct {
    char    nome[32];
    Vector2 pos;              /* centro do mago */
    Vector2 mira;             /* direção (tamanho 1) para onde ele aponta */
    Vector2 empurrao;         /* velocidade extra quando leva dano; some aos poucos */
    Vector2 dir_esquiva;
    float   raio;             /* raio de colisão */
    float   velocidade;       /* pixels por segundo */
    int     vida;
    int     vida_max;
    int     ataque;
    float   recarga_esfera;       /* tempo até poder lançar de novo (0 = pronto) */
    float   tempo_entre_esferas;
    float   invulneravel;         /* tempo restante sem poder levar dano */
    float   esquivando;           /* tempo restante da esquiva */
    float   recarga_esquiva;
    float   tempo_recarga_esquiva;  /* segundos entre uma esquiva e outra */
} Personagem;

/* Melhorias que o jogador escolhe ao subir de andar */
typedef enum {
    MELHORIA_VIDA = 0,
    MELHORIA_CURA,
    MELHORIA_DANO,
    MELHORIA_RECARGA,
    MELHORIA_VELOCIDADE,
    MELHORIA_ESQUIVA,
    TOTAL_MELHORIAS
} TipoMelhoria;

#define OFERTAS_MELHORIA 3

typedef enum {
    ECO_CACADOR = 0,   /* corre atrás do mago e machuca no contato */
    ECO_ATIRADOR       /* mantém distância e atira */
} TipoEco;

typedef struct {
    char    nome[32];
    TipoEco tipo;
    Vector2 pos;
    Vector2 empurrao;
    float   raio;
    float   velocidade;
    int     vida;
    int     vida_max;
    int     ataque;
    float   recarga_tiro;   /* só o atirador usa */
    int     sentido;        /* atirador anda de lado: +1 ou -1 (troca ao bater) */
    float   surgindo;       /* enquanto > 0 o eco está aparecendo: não age nem leva dano */
    float   flash;          /* brilho rápido quando leva dano */
    int     lembranca;      /* 1 = eco de alguém que caiu neste andar (mais forte, tem nome) */
} Eco;

/* Vetor dinâmico de ecos (alocado com calloc/realloc, igual na unidade I). */
typedef struct {
    Eco *itens;
    int  total;
    int  capacidade;
} EquipeEcos;

typedef struct {
    Vector2 pos;
    Vector2 vel;            /* pixels por segundo */
    float   raio;
    int     dano;
    int     do_jogador;     /* 1 = esfera do mago, 0 = tiro de eco */
    float   tempo_vida;     /* some quando chega a 0 */
    int     ativo;
} Projetil;

typedef struct {
    Projetil itens[MAX_PROJETEIS];
} GerenciadorProjeteis;

/* ---------- Efeitos visuais ---------- */

typedef struct {
    int     valor;
    Vector2 pos;
    float   tempo_restante;
    float   tempo_total;
    int     ativo;
} MensagemDano;

typedef struct {
    Vector2 pos;
    Color   cor;
    float   tamanho;
    float   tempo_restante;
    float   tempo_total;
    int     ativo;
} Impacto;

typedef struct {
    MensagemDano mensagens[MAX_MENSAGENS];
    Impacto      impactos[MAX_IMPACTOS];
} GerenciadorEfeitos;

/* ---------- Partida ---------- */

typedef struct {
    EstadoJogo   estado;
    int          andar;            /* andar atual (1 = primeiro) */
    int          andar_max;
    int          onda;             /* onda dentro do andar (0 = ainda não começou) */
    int          ondas_no_andar;
    int          escada_aberta;    /* 1 quando todas as ondas do andar acabaram */
    TipoMelhoria ofertas[OFERTAS_MELHORIA];
    float        tempo_partida;    /* segundos jogados nesta partida */
    float        intervalo;        /* contagem até a próxima onda (0 = não está em intervalo) */
    float        tempo_titulo;     /* nome do andar aparecendo na tela */
    int          ecos_derrotados;
    char         mensagem[160];    /* último acontecimento, mostrado no rodapé */
    float        tempo_mensagem;
    int          debug;            /* F1 liga/desliga o painel de desempenho */
    char         jogador[TAM_NOME];  /* nome digitado antes de começar */
    int          bonus;            /* pontos extras (ecos lembrança derrotados) */
    int          pontos;           /* pontuação final (calculada quando a partida acaba) */
    int          posicao;          /* lugar no ranking (1 = primeiro) */
    int          total_partidas;   /* quantas partidas existem no histórico */
} Jogo;

/* Contadores para medir o custo dos algoritmos a cada quadro (modo debug). */
typedef struct {
    int  comparacoes_ordenacao;   /* Insertion Sort da ordem de desenho */
    long testes_separacao;        /* pares de ecos testados (laço duplo) */
} Desempenho;

/* Resumo calculado a partir do histórico (percorrendo todos os registros). */
typedef struct {
    int partidas;
    int vitorias;
    int derrotas;
    int melhor_andar;             /* andar mais alto já alcançado */
    int melhor_pontos;
} Estatisticas;

/* ---------- Histórico (Fase 3) ---------- */

/*
 * Um registro por partida, gravado em assets/dados/historico.bin.
 * Todos os campos têm tamanho fixo: assim o arquivo é um vetor de
 * registros iguais, e dá para saber quantos existem dividindo o tamanho
 * do arquivo por sizeof(RegistroPartida).
 */
typedef struct {
    char      nome[TAM_NOME];
    int       venceu;           /* 1 = chegou ao topo */
    int       andar;            /* andar alcançado */
    int       onda;             /* onda em que estava quando a partida acabou */
    int       tempo;            /* segundos */
    int       ecos_derrotados;
    int       pontos;
    long long data;             /* time(NULL) de quando a partida acabou */
} RegistroPartida;

/* Vetor dinâmico com todas as partidas (fica ordenado por pontos). */
typedef struct {
    RegistroPartida *itens;
    int total;
    int capacidade;
} Historico;

/* Por qual informação ordenar o ranking. */
typedef enum {
    CRITERIO_PONTOS = 0,   /* mais pontos primeiro */
    CRITERIO_ANDAR,        /* andar mais alto primeiro (empate: mais rápido) */
    CRITERIO_NOME,         /* ordem alfabética */
    CRITERIO_DATA,         /* mais recentes primeiro */
    TOTAL_CRITERIOS
} Criterio;

typedef enum {
    ALGORITMO_MERGE = 0,
    ALGORITMO_QUICK
} AlgoritmoOrdenacao;

#endif
