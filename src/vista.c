#include <vista.h>

#include <shmState.h>
#include <shmSync.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int main(int argc, char *argv[]){
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <height> <width>\n", argv[0]);
        return 1;
    }

    size_t height = (size_t)atoi(argv[1]);
    size_t width = (size_t)atoi(argv[2]);

    int syncFd = -1;
    int stateFd = -1;
    semaphoresStatus *sync = NULL;
    GameState *state = NULL;

    if (syncOpen(&syncFd, &sync) == -1) {
        perror("syncOpen");
        return 1;
    }
    if (stateOpen(&stateFd, &state, width, height) == -1) {
        perror("stateOpen");
        return 1;
    }

    while (!state->gameOver) {
        /* Si el master inicializa semáforos, esta espera sincroniza el render. */
        (void)sync; /* evita warning si todavía no se usan semáforos */

        clearScreen();
        printView(state->board, height, width);
        printStats(state);

        /* En una implementación completa: sem_post(&sync->showDone); */
        usleep(50 * 1000);
    }

    (void)syncClose(syncFd, sync);
    (void)stateClose(stateFd, state, width, height);
    return 0;
}

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

void printView(int *board, size_t height, size_t width){
    printf("%s", kThemeBase);
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            printf("%d", board[i * width + j]);
        }
        printf("\n");
    }
    printf("%s", kReset);
}

void clearScreen(){
    /* Tema + limpiar pantalla + cursor a (0,0) */
    const char *clear = "\033[0m\033[48;5;236m\033[38;5;252m\033[2J\033[H";
    write(STDOUT_FILENO, clear, strlen(clear));
}
