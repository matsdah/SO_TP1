#include <viewRender.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* 
** viewRender.c -> render del tablero y estadísticas con colores/símbolos. 
*/

static const char *K_THEME_BASE = "\033[0m\033[48;5;236m\033[38;5;252m";
static const char *K_RESET = "\033[0m";

/* Colores para el texto. */
static const char *STATS_COLORS[] = {
    "\033[48;5;236m\033[38;5;110m",
    "\033[48;5;236m\033[38;5;150m",
    "\033[48;5;236m\033[38;5;180m",
    "\033[48;5;236m\033[38;5;174m",
    "\033[48;5;236m\033[38;5;175m",
    "\033[48;5;236m\033[38;5;139m",
    "\033[48;5;236m\033[38;5;109m",
    "\033[48;5;236m\033[38;5;246m",
    "\033[48;5;236m\033[38;5;215m"
};

/* Colores para el trazo de cada jugador en el tablero. */
static const char *TRAIL_COLORS[] = {
    "\033[48;5;17m\033[38;5;117m",
    "\033[48;5;22m\033[38;5;157m",
    "\033[48;5;94m\033[38;5;229m",
    "\033[48;5;94m\033[38;5;216m",
    "\033[48;5;53m\033[38;5;219m",
    "\033[48;5;54m\033[38;5;183m",
    "\033[48;5;23m\033[38;5;159m",
    "\033[48;5;238m\033[38;5;251m",
    "\033[48;5;52m\033[38;5;217m"
};

static const size_t STATS_COLORS_COUNT = sizeof(STATS_COLORS) / sizeof(STATS_COLORS[0]);
static const size_t TRAIL_COLORS_COUNT = sizeof(TRAIL_COLORS) / sizeof(TRAIL_COLORS[0]);

static const char *HEAD_SYMBOL = " 😃 ";        /* Símbolo para la cabeza de cada jugador */
static const char *TRAIL_SYMBOL = " ⬜ ";       /* Símbolo para el trazo de cada jugador */

static int writeEscapeSequence(const char *sequence){
    size_t total = strlen(sequence);
    size_t written = 0;

    while(written < total){
        ssize_t result = write(STDOUT_FILENO, sequence + written, total - written);

        if(result == -1){
            if(errno == EINTR){
                continue;
            }

            perror("Error escribiendo secuencia ANSI.");
            return -1;
        }
        written += (size_t)result;
    }

    return 0;
}

void enterAlternateScreen(){
    /* Secuencia de escape para entrar en la pantalla alternativa. */
    const char *enter = "\033[?1049h\033[2J\033[H";
    writeEscapeSequence(enter);
}

void leaveAlternateScreen(){
    /* Secuencia de escape para salir de la pantalla alternativa. */
    const char *leave = "\033[?1049l\033[0m";
    writeEscapeSequence(leave);
}

void printPlayerStats(const Player *playerState){
    if(playerState == NULL){
        fprintf(stderr, "Error: playerState nulo en printPlayerStats.\n");
        return;
    }

    printf("👤 Nombre: %s\t💯 Puntaje: %u\t✅ Movimientos válidos: %u\t❌ Movimientos inválidos: %u\t📍 Coordenadas: (%u,%u)\n",
           playerState->name,
           playerState->score,
           playerState->validMoves,
           playerState->invalidMoves,
           (unsigned int)playerState->x,
           (unsigned int)playerState->y);
}

void printStats(const GameState *gameState){
    if(gameState == NULL){
        fprintf(stderr, "Error: gameState nulo en printStats.\n");
        return;
    }

    for(size_t i = 0; i < gameState->playerCount; i++){
        printf("%s", STATS_COLORS[i % STATS_COLORS_COUNT]);
        printPlayerStats(&(gameState->players)[i]);
        printf("%s", K_RESET);
    }
}

void printView(const GameState *gameState){
    if(gameState == NULL){
        fprintf(stderr, "Error: gameState nulo en printView.\n");
        return;
    }

    printf("%s", K_THEME_BASE);

    /* Imprime el tablero. */
    for(size_t i = 0; i < gameState->height; i++){
        for(size_t j = 0; j < gameState->width; j++){

            int value = BOARD_AT(gameState->board, gameState->width, i, j);

            if(value < 0){
                /* Procesa el valor negativo para identificar al jugador. */
                int playerIndex = -(value + 1);

                bool isHead = false;

                if((playerIndex >= 0) && ((size_t)playerIndex < gameState->playerCount)){
                    /* Verifica si es la cabeza del jugador. */
                    if((gameState->players[playerIndex].x == j) && (gameState->players[playerIndex].y == i)){
                        isHead = true;
                    }
                }

                /* Imprime el símbolo del jugador. */
                if((playerIndex >= 0) && ((size_t)playerIndex < gameState->playerCount)){

                    printf("%s", TRAIL_COLORS[playerIndex % TRAIL_COLORS_COUNT]);

                    if(isHead){
                        printf("%s", HEAD_SYMBOL);
                    }else{
                        printf("%s", TRAIL_SYMBOL);
                    }
                    printf("%s", K_THEME_BASE);
                }else{
                    printf(" ?? ");
                }
            }else{
                printf(" %-2d ", value);
            }
        }
        printf("\n");
    }

    printf("\n");

    /* Imprime la información de cada jugador. */
    for(size_t p = 0; p < gameState->playerCount; p++){
        printf("%s", TRAIL_COLORS[p % TRAIL_COLORS_COUNT]);
        printf("P%zu", p + 1);
        printf("%s", K_THEME_BASE);
        printf(" = (%u,%u) ", (unsigned int)gameState->players[p].x, (unsigned int)gameState->players[p].y);
    }
    printf("\n");

    printf("%s", K_RESET);
}

void clearScreen(){
    /* Secuencia de escape ANSI para limpiar la pantalla y mover el cursor a la posición (0,0). */
    const char *clear = "\033[0m\033[48;5;236m\033[38;5;252m\033[2J\033[H";
    writeEscapeSequence(clear);
}
