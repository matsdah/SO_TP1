#include "../include/vista.h"
#include "../include/structures.h"
#include <unistd.h>
#include <string.h>

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


/* Función principal de la vista. */
int main(int argc, char const *argv[]){
    unsigned int heigth;
    unsigned int width;

    
    heigth = atoi(argv[1]); /* Ancho del tablero */
    width = atoi(argv[2]);  /* Alto del tablero */


    semaphoresStatus* semStatus = createSHM("/game_sync",sizeof(semaphoresStatus));
    GameState* gStatus = createSHM("/game_state", sizeof(GameState) + (sizeof(int)  *(heigth * width)));


    while(!gStatus->gameOver){
        // wait_view(semStatus);   falta implementar semaforos para esta funcion
        clearScreen();
        printView(gStatus->board, heigth, width);
        printStats(gStatus);
        // signal_view(semStatus); falta implementar semaforos para esta funcion
    }

    return 0;
}


void printPlayerStats(Player* player_state){

    printf("Name:%s\tPoints:%d\t#ValidM:%d\t#InvalidM:%d\tCoords:(%d,%d)\n", player_state->playerName, player_state->playerScore, player_state->validMovements, player_state->invalidMovements, player_state->x, player_state->y);

}


void printStats(GameState * gameState){

    for(size_t i = 0; i < gameState->playersCount; i++){
        printf("%s",colors[i]);
        printPlayerStats(&(gameState->players)[i]);
    }
}


void printView(int * board,size_t height,size_t width){
    size_t i, j;
    for(i = 0; i < width; i++){

        for(j = 0; j < height; j++){
            printf("%d", board[i * width + j]);
        }

        printf("\n");
    }
}


void clearScreen(){
     /* Código ANSI para limpiar pantalla y mover el cursor a la esquina superior izquierda */
    const char *clear = "\033[2J\033[H";

    write(STDOUT_FILENO, clear, strlen(clear));
}