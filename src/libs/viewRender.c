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

/* Colores para texto de stats */
static const char *statsColors[] = {
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

/* Colores para el trazo/rastro de cada jugador en el tablero */
static const char *trailColors[] = {
    "\033[48;5;17m\033[38;5;117m",  /* azul oscuro/claro */
    "\033[48;5;22m\033[38;5;157m",  /* verde oscuro/claro */
    "\033[48;5;94m\033[38;5;229m",  /* amarillo oscuro/claro */
    "\033[48;5;94m\033[38;5;216m",  /* naranja oscuro/claro */
    "\033[48;5;53m\033[38;5;219m",  /* rosa oscuro/claro */
    "\033[48;5;54m\033[38;5;183m",  /* morado oscuro/claro */
    "\033[48;5;23m\033[38;5;159m",  /* cyan oscuro/claro */
    "\033[48;5;238m\033[38;5;251m", /* gris oscuro/claro */
    "\033[48;5;52m\033[38;5;217m"   /* melocotón oscuro/claro */
};

/* Símbolos para cabeza (H) y rastro (#) con ancho fijo */
static const char *HEAD_SYMBOL = " H  ";
static const char *TRAIL_SYMBOL = " #  ";

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
        printf("%s", statsColors[i % (sizeof(statsColors) / sizeof(statsColors[0]))]);
        printPlayerStats(&(gameState->players)[i]);
        printf("%s", kReset);
    }
}

void printView(const GameState *gameState){
    printf("%s", kThemeBase);
    
    for (size_t i = 0; i < gameState->height; i++) {
        for (size_t j = 0; j < gameState->width; j++) {
            int value = BOARD_AT(gameState->board, gameState->width, i, j);
            
            /* Verificar si es una celda ocupada (negativa) */
            if (value < 0) {
                /* Valor negativo: -(playerIndex + 1) */
                int playerIndex = -(value + 1);
                
                /* Verificar si es la cabeza del jugador */
                int isHead = 0;
                if (playerIndex >= 0 && playerIndex < (int)gameState->playersCount) {
                    if (gameState->players[playerIndex].x == j && 
                        gameState->players[playerIndex].y == i) {
                        isHead = 1;
                    }
                }
                
                /* Imprimir con color del jugador */
                if (playerIndex >= 0 && playerIndex < CANT_PLAYERS) {
                    printf("%s", trailColors[playerIndex % (sizeof(trailColors) / sizeof(trailColors[0]))]);
                    if (isHead) {
                        printf("%s", HEAD_SYMBOL);
                    } else {
                        printf("%s", TRAIL_SYMBOL);
                    }
                    printf("%s", kThemeBase);
                }
            } else {
                /* Celda libre: mostrar valor con ancho fijo de 4 */
                printf(" %-2d ", value);
            }
        }
        printf("\n");
    }

    /* Mostrar posiciones de jugadores */
    printf("\n");
    for (size_t p = 0; p < gameState->playersCount; p++) {
        printf("%s", trailColors[p % (sizeof(trailColors) / sizeof(trailColors[0]))]);
        printf("P%zu", p + 1);
        printf("%s", kThemeBase);
        printf("=(%u,%u) ",
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
