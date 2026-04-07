#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>

/* Dimensión máxima del nombre de un jugador */
#define NAME_DIM 16

/* Número máximo de jugadores */
#define CANT_PLAYERS 9

/* Macro para calcular el tamaño del estado del juego */
#define GAME_STATUS_SIZE(GameState, width, height) (sizeof(GameState) + (sizeof(int) * (width * height)))

/* Macro para verificar si una posición es la cabeza de un jugador */
#define IS_HEAD(x, y, j, i) ((x) == (j) && (y) == (i))

/* Macro para acceder a una posición del tablero */
#define BOARD_AT(board, w, i, j) ((board)[((i) * (w)) + (j)])


/* Estructura para representar el estado del juego */
typedef struct{
    char playerName[NAME_DIM];
    unsigned int playerScore;
    unsigned int validMovements;
    unsigned int invalidMovements;
    unsigned short x;                   /* Posicion en el tablero */
    unsigned short y;
    pid_t playerPID;                    /* PID del proceso del jugador */
    bool valid;                         /* Indica si el jugador tiene movimientos válidos */
} Player;


/* Estructura para representar el estado del tablero */
typedef struct{
    unsigned short width;               /* Ancho del tablero */
    unsigned short height;              /* Alto del tablero */
    unsigned char playersCount;         /* Cantidad de jugadores en el juego */
    Player players[CANT_PLAYERS];       /* Información de los jugadores */
    bool gameOver;                      /* Indica si el juego ha terminado */
    int board[];        /* Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1 */   
} GameState;


/* Estructura para representar el estado de los semáforos */
typedef struct{ 
    sem_t printNeeded;           /* Indicarle a la vista que hay cambios por imprimir */
    sem_t renderDone;             /* Indicarle al master que la vista terminó de imprimir */
    sem_t masterMutex;          /* Mutex para evitar inanición del master al acceder al estado */
    sem_t gameStateMutex;       /* Mutex para evitar condiciones de carrera. */
    sem_t readCountMutex;    /* Mutex para la siguiente variable */

    /* Cantidad de jugadores leyendo el estado */
    unsigned int playersReadingStatus;
    sem_t playersAllowedToMove [CANT_PLAYERS]; /* Le indica a cada jugador que puede enviar 1 movimiento */
} semaphoresStatus;


/* Parameters values. */ 
typedef struct{
    size_t width;       /* Ancho del tablero */
    size_t height;      /* Alto del tablero */
    size_t delay;       /* Retraso en milisegundos */
    size_t timeout;     /* Timeout en segundos */
    size_t seed;        /* Semilla para la generación del tablero */
    char * view;        /* Ruta del binario de la vista */
    char * players[CANT_PLAYERS];   /* Rutas de los binarios de los jugadores */
    size_t amount_players;          /* Cantidad de jugadores */
} Parameters;


// To help parsing parameters and their functions
typedef int (*ParamHandler)(const char *value, void *context);


typedef struct{
    const char *flag;
    int expects_value;
    ParamHandler handler;
    const char *help;
} Parameter;


typedef struct {
    pid_t pid;
    int pipeFd;
} PlayerProcess;

#endif