#include <viewRender.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*
 * Paleta más moderna y apagada (256-colors).
 * Fondo común oscuro + colores de texto suaves por jugador.
 */
static const char *kThemeBase = "\033[0m\033[48;5;236m\033[38;5;252m"; /* bg: gris oscuro, fg: gris claro */
static const char *kReset = "\033[0m";

static const char *colors[] = {
    "\033[48;5;236m\033[38;5;110m", /* muted blue */
    "\033[48;5;236m\033[38;5;150m", /* muted green */
    "\033[48;5;236m\033[38;5;180m", /* muted yellow */
    "\033[48;5;236m\033[38;5;174m", /* muted orange */
    "\033[48;5;236m\033[38;5;175m", /* muted pink */
    "\033[48;5;236m\033[38;5;139m", /* muted purple */
    "\033[48;5;236m\033[38;5;109m", /* muted cyan */
    "\033[48;5;236m\033[38;5;246m", /* muted gray */
    "\033[48;5;236m\033[38;5;215m"  /* muted peach */
};

void printPlayerStats(Player *player_state){
    printf("Name:%s\tPoints:%u\t#ValidM:%u\t#InvalidM:%u\tCoords:(%u,%u)\n",
           player_state->playerName,
           player_state->playerScore,
           player_state->validMovements,
           player_state->invalidMovements,
           player_state->x,
           player_state->y);
}

void printStats(GameState *gameState){
    for (size_t i = 0; i < gameState->playersCount; i++) {
        printf("%s", colors[i % (sizeof(colors) / sizeof(colors[0]))]);
        printPlayerStats(&(gameState->players)[i]);
        printf("%s", kReset);
    }
}

void printView(const GameState *gameState){
    printf("%s", kThemeBase);
    for (size_t i = 0; i < gameState->height; i++) {
        for (size_t j = 0; j < gameState->width; j++) {
            int value = BOARD_AT(gameState->board, gameState->width, i, j);
            printf("%4d", value);
        }
        printf("\n");
    }

    for (size_t p = 0; p < gameState->playersCount; p++) {
        printf("P%zu=(%u,%u) ",
               p + 1,
               gameState->players[p].x,
               gameState->players[p].y);
    }
    printf("\n");

    printf("%s", kReset);
}

void clearScreen(){
    /* Tema + limpiar pantalla + cursor a (0,0) */
    const char *clear = "\033[0m\033[48;5;236m\033[38;5;252m\033[2J\033[H";
    write(STDOUT_FILENO, clear, strlen(clear));
}
