#include <sys/wait.h>

#define NAME_DIM 20
#define CANT_PLAYERS 9

/* Estructura para representar el estado del juego */
typedef struct{
    char playerName[NAME_DIM];
    unsigned int playerScore;
    unsigned int validMovements;
    unsigned int invalidMovements;
    unsigned int x;                     /* Posicion en el tablero */
    unsigned int y;
    pid_t playerPID;                    /* PID del proceso del jugador */
    bool valid;                         /* Indica si el jugador tiene movimientos válidos */
} Player;

/* Estructura para representar el estado del tablero */
typedef struct{
    unsigned int width;                      /* Ancho del tablero */
    unsigned int height;                     /* Alto del tablero */
    Player players[CANT_PLAYERS];            /* Información de los jugadores */
    bool gameOver;                      /* Indica si el juego ha terminado */
} GameState;

