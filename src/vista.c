#include "../include/vista.h"
#include "../include/structures.h"
#include <unistd.h>
#include <string.h>

#define GREY    "\x1b[48;5;236m"    // Fondo gris oscuro
#define WHITE   "\x1b[38;5;15m"     // Letra blanca
#define BLUE    "\x1b[38;5;39m"     // Letra azul
#define RESET_COLOR "\033[0;0m"     // Restablecer color

/* Vector de códigos de color para todos los jugadores. */
const char *colors[] = {
    "\033[40;97m",  // Fondo negro, letra blanca
    "\033[41;97m",  // Fondo rojo, letra blanca
    "\033[42;30m",  // Fondo verde, letra negra
    "\033[43;30m",  // Fondo amarillo, letra negra
    "\033[44;97m",  // Fondo azul, letra blanca
    "\033[45;97m",  // Fondo magenta, letra blanca
    "\033[46;30m",  // Fondo cian, letra negra
    "\033[47;30m",  // Fondo blanco, letra negra
    "\033[100;97m"  // Fondo gris claro, letra blanca
};


const char *headColors[] = {
    "\033[48;5;235;97m",  // Cabeza con fondo gris más oscuro y letra blanca
    "\033[48;5;196;97m",  // Cabeza con fondo rojo y letra blanca
    "\033[48;5;28;97m",   // Cabeza con fondo verde oscuro y letra blanca
    "\033[48;5;130;97m",  // Cabeza con fondo marrón oscuro y letra blanca
    "\033[48;5;32;97m",   // Cabeza con fondo azul y letra blanca
    "\033[48;5;125;97m",  // Cabeza con fondo magenta oscuro y letra blanca
    "\033[48;5;51;97m",   // Cabeza con fondo cian oscuro y letra blanca
    "\033[48;5;15;97m",   // Cabeza con fondo blanco y letra negra
    "\033[48;5;237;97m"   // Cabeza con fondo gris oscuro y letra blanca
};


/* Función principal de la vista. */
int main(int argc, char const *argv[]){
    unsigned int heigth;
    unsigned int width;

    
    heigth = atoi(argv[1]); /* Ancho del tablero */
    width = atoi(argv[2]);  /* Alto del tablero */


    // Tgame_sync * game_sync = get_game_sync();  SEMAFOROS
    
    // GameState * gameState = get_game_state(GAME_STATUS_SIZE(game_state, width, height));  SEMAFOROS


    while(!gameState->gameOver){
        // wait_view(semStatus);   falta implementar semaforos para esta funcion
        clearScreen();
        printView(gameState->board, heigth, width, gameState);
        printStats(gStatus);
        // signal_view(semStatus); falta implementar semaforos para esta funcion
    }

    return 0;
}


void printPlayerStats(Player* player_state){
    printf("Name:%s\tPoints:%d\t#ValidM:%d\t#InvalidM:%d\tCoords:(%d,%d)\n", player_state->playerName, player_state->playerScore, player_state->validMovements, player_state->invalidMovements, player_state->x, player_state->y);
}


void printStats(GameState * gameState){
    size_t i;

    for(i = 0; i < gameState->playersCount; i++){
        printf("%s",colors[i]);
        printPlayerStats(&(gameState->players)[i]);
    }
}


void printView(int * board, size_t height, size_t width, GameState * gameState){
    size_t i, j, k;

    for(i = 0; i < height; i++){
        printf("%s%s  *  %s", GREY, WHITE, RESET_COLOR);

        for (j = 0; j < width; j++){
            if(BOARD_AT(board, width, i, j) <= 0){
                k = (-1) * BOARD_AT(board, width, i, j);
                printf("%s     ", IS_HEAD(gameState->players[k].x, gameState->players[k].y, j, i) ? colorescabeza[k] : colores[k]);
            } else {
                printf("| %d |", BOARD_AT(board, width, i, j));
            } 
            printf("%s", RESET_COLOR);
        }

        printf("%s%s  *  %s\n", GREY, WHITE, RESET_COLOR);
    }
}


void clearScreen(){
     /* Código ANSI para limpiar pantalla y mover el cursor a la esquina superior izquierda */
    const char *clear = "\033[2J\033[H";

    write(STDOUT_FILENO, clear, strlen(clear));
}