#ifndef UI_H
#define UI_H

#include "jogo.h"
#include "partida.h"
#include "ranking.h"

void desenhar_hud(const Partida *p);
void desenhar_rodape(const Partida *p);
void desenhar_mira(void);
void desenhar_aviso_intervalo(const Jogo *j);
void desenhar_titulo_andar(const Partida *p);
void desenhar_aviso_arquivo(const Partida *p);
void desenhar_debug(const Partida *p);

/* Retângulo (na tela) da carta de melhoria i — usado no desenho e no clique. */
Rectangle retangulo_carta(int i);

void desenhar_tela_menu(const Estatisticas *stats, const char *aviso_historico);
void desenhar_tela_nome(const char *nome);
void desenhar_tela_ranking(const TelaRanking *t);
void desenhar_tela_pausa(void);
void desenhar_tela_melhoria(const Partida *p);
void desenhar_tela_vitoria(const Estatisticas *stats, const Partida *p);
void desenhar_tela_derrota(const Estatisticas *stats, const Partida *p);

#endif
