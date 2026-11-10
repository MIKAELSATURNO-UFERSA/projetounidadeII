#include <raylib.h>
#include <time.h>
#include <string.h>
#include "jogo.h"
#include "partida.h"
#include "personagem.h"
#include "onda.h"
#include "historico.h"
#include "diario.h"
#include "ranking.h"
#include "ui.h"
#include "assets.h"

#define DT_MAXIMO (1.0f / 30.0f)   /* se o PC travar um instante, o jogo não "teleporta" */

/*
 * Lê as teclas digitadas neste quadro e coloca no texto (nome do jogador,
 * busca do ranking). Aceita só caracteres comuns, sem acento, porque a
 * fonte padrão da raylib pode não ter as letras acentuadas.
 */
static void editar_texto(char *texto, int tamanho) {
    int n = (int)strlen(texto);
    int c = GetCharPressed();
    while (c > 0) {
        int valido = (c >= 32 && c <= 126) && !(c == ' ' && n == 0);
        if (valido && n < tamanho - 1) {
            texto[n++] = (char)c;
            texto[n] = '\0';
        }
        c = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && n > 0) {
        texto[--n] = '\0';
    }
}

static void limpar_fila_de_teclas(void) {
    while (GetCharPressed() > 0) { }
}

int main(void) {
    InitWindow(LARGURA_TELA, ALTURA_TELA, "O Ultimo Andar");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   /* o ESC é tratado por tela (nas telas de texto ele volta ao menu) */
    /* time(NULL) muda a cada execução; GetTime() logo depois de abrir a
       janela é quase sempre ~0, e aí os sorteios se repetiam. */
    SetRandomSeed((unsigned int)time(NULL));

    Assets assets;
    if (!carregar_assets(&assets)) {
        CloseWindow();
        return 1;
    }

    /* Histórico: lê todas as partidas do arquivo binário. */
    Historico historico;
    historico_iniciar(&historico);
    char aviso_historico[160];
    historico_carregar(&historico, aviso_historico, sizeof(aviso_historico));
    Estatisticas stats = calcular_estatisticas(&historico);

    char nome[TAM_NOME] = "";
    ler_ultimo_jogador(nome, sizeof(nome));

    Partida partida;
    preparar_partida(&partida, &historico);

    TelaRanking ranking;
    memset(&ranking, 0, sizeof(ranking));

    /* A câmera só desloca a sala para baixo do HUD (e treme quando o mago apanha). */
    Camera2D camera = { 0 };
    camera.zoom = 1.0f;

    int sair = 0;
    while (!sair && !WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > DT_MAXIMO) dt = DT_MAXIMO;

        Jogo *j = &partida.jogo;
        EstadoJogo estado_antes = j->estado;

        if (IsKeyPressed(KEY_F1)) j->debug = !j->debug;

        /* ---------------- lógica ---------------- */
        switch (j->estado) {
        case ESTADO_MENU:
            if (IsKeyPressed(KEY_ESCAPE)) sair = 1;
            if (IsKeyPressed(KEY_ENTER)) {
                limpar_fila_de_teclas();
                j->estado = ESTADO_NOME;
            }
            if (IsKeyPressed(KEY_R)) {
                limpar_fila_de_teclas();
                abrir_ranking(&ranking, &historico);
                j->estado = ESTADO_RANKING;
            }
            break;

        case ESTADO_NOME:
            editar_texto(nome, TAM_NOME);
            if (IsKeyPressed(KEY_ESCAPE)) j->estado = ESTADO_MENU;
            if (IsKeyPressed(KEY_ENTER)) {
                if (nome[0] == '\0') strcpy(nome, "Mago");
                salvar_ultimo_jogador(nome);
                reiniciar_partida(&partida, nome);
            }
            break;

        case ESTADO_RANKING:
            editar_texto(ranking.busca, TAM_NOME);
            if (IsKeyPressed(KEY_RIGHT)) {
                ranking.criterio = (Criterio)((ranking.criterio + 1) % TOTAL_CRITERIOS);
                reordenar_ranking(&ranking);
            }
            if (IsKeyPressed(KEY_LEFT)) {
                ranking.criterio = (Criterio)((ranking.criterio + TOTAL_CRITERIOS - 1) % TOTAL_CRITERIOS);
                reordenar_ranking(&ranking);
            }
            if (IsKeyPressed(KEY_TAB)) {
                ranking.algoritmo = (ranking.algoritmo == ALGORITMO_MERGE) ? ALGORITMO_QUICK
                                                                           : ALGORITMO_MERGE;
                reordenar_ranking(&ranking);
            }
            if (IsKeyPressed(KEY_ENTER)) buscar_no_ranking(&ranking);
            if (IsKeyPressed(KEY_ESCAPE)) {
                fechar_ranking(&ranking);
                j->estado = ESTADO_MENU;
            }
            break;

        case ESTADO_JOGANDO:
            if (IsKeyPressed(KEY_ESCAPE)) { sair = 1; break; }
            if (IsKeyPressed(KEY_P)) {
                j->estado = ESTADO_PAUSADO;
                break;
            }
            if (j->debug && IsKeyPressed(KEY_F2)) {
                spawnar_ecos_extras(&partida.ecos, 10, j->andar, &partida.andar.mapa, &partida.mago);
            }
            if (j->debug && IsKeyPressed(KEY_F3)) {
                recarregar_andar(&partida);   /* relê o .txt do andar: bom para testar edições */
            }
            atualizar_partida(&partida, ler_controles(), dt);
            break;

        case ESTADO_PAUSADO:
            if (IsKeyPressed(KEY_ESCAPE)) sair = 1;
            if (IsKeyPressed(KEY_P)) j->estado = ESTADO_JOGANDO;
            break;

        case ESTADO_MELHORIA: {
            if (IsKeyPressed(KEY_ESCAPE)) { sair = 1; break; }
            int escolha = -1;
            if (IsKeyPressed(KEY_ONE))   escolha = 0;
            if (IsKeyPressed(KEY_TWO))   escolha = 1;
            if (IsKeyPressed(KEY_THREE)) escolha = 2;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                for (int i = 0; i < OFERTAS_MELHORIA; i++) {
                    if (CheckCollisionPointRec(GetMousePosition(), retangulo_carta(i))) escolha = i;
                }
            }
            if (escolha >= 0) escolher_melhoria(&partida, escolha);
            break;
        }

        case ESTADO_VITORIA:
        case ESTADO_DERROTA:
            if (IsKeyPressed(KEY_ESCAPE)) sair = 1;
            if (IsKeyPressed(KEY_ENTER)) j->estado = ESTADO_MENU;
            break;
        }

        /* A partida acabou neste quadro: recalcula as estatísticas do menu. */
        if (estado_antes == ESTADO_JOGANDO &&
            (j->estado == ESTADO_VITORIA || j->estado == ESTADO_DERROTA)) {
            stats = calcular_estatisticas(&historico);
        }

        /* Durante o jogo o cursor do sistema some e fica só a mira desenhada. */
        if (j->estado == ESTADO_JOGANDO && !IsCursorHidden()) HideCursor();
        if (j->estado != ESTADO_JOGANDO && IsCursorHidden())  ShowCursor();

        camera.offset = (Vector2){ 0.0f, (float)ALTURA_HUD };
        if (partida.tremor > 0.0f && j->estado == ESTADO_JOGANDO) {
            float forca = 30.0f * partida.tremor;   /* até ~6 px, diminuindo */
            camera.offset.x += GetRandomValue(-100, 100) / 100.0f * forca;
            camera.offset.y += GetRandomValue(-100, 100) / 100.0f * forca;
        }

        /* ---------------- desenho ---------------- */
        BeginDrawing();
        ClearBackground((Color){ 10, 10, 20, 255 });

        if (j->estado == ESTADO_MENU) {
            desenhar_tela_menu(&stats, aviso_historico);

        } else if (j->estado == ESTADO_NOME) {
            desenhar_tela_nome(nome);

        } else if (j->estado == ESTADO_RANKING) {
            desenhar_tela_ranking(&ranking);

        } else if (j->estado == ESTADO_VITORIA) {
            desenhar_tela_vitoria(&stats, &partida);

        } else if (j->estado == ESTADO_DERROTA) {
            desenhar_tela_derrota(&stats, &partida);

        } else {
            BeginMode2D(camera);
            desenhar_mundo(&partida, &assets);
            EndMode2D();

            desenhar_hud(&partida);
            desenhar_rodape(&partida);
            desenhar_aviso_arquivo(&partida);

            /* onda 0 = começo do andar: no lugar do "onda limpa" aparece o título */
            if (j->intervalo > 0.0f && j->onda > 0) desenhar_aviso_intervalo(j);
            desenhar_titulo_andar(&partida);
            if (j->debug) desenhar_debug(&partida);

            if (j->estado == ESTADO_PAUSADO)        desenhar_tela_pausa();
            else if (j->estado == ESTADO_MELHORIA)  desenhar_tela_melhoria(&partida);
            else                                    desenhar_mira();
        }

        EndDrawing();
    }

    fechar_ranking(&ranking);
    liberar_partida(&partida);
    historico_liberar(&historico);
    liberar_assets(&assets);
    CloseWindow();
    return 0;
}
