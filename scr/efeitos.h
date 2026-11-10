#ifndef EFEITOS_H
#define EFEITOS_H

#include "jogo.h"

void inicializar_efeitos(GerenciadorEfeitos *g);

/* Número que sobe e some (ex.: -5 em vermelho). */
void adicionar_mensagem(GerenciadorEfeitos *g, Vector2 pos, int valor);

/* Anel que se expande onde algo bateu ou explodiu. */
void adicionar_impacto(GerenciadorEfeitos *g, Vector2 pos, Color cor, float tamanho);

void atualizar_efeitos(GerenciadorEfeitos *g, float dt);
void desenhar_efeitos(const GerenciadorEfeitos *g);

#endif
