#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>

/* Dimension maxima del nombre de un jugador. */
#define NAME_DIM 16

/* Numero maximo de jugadores. */
#define CANT_PLAYERS 9

/* Macro para calcular el tamano del estado del juego. */
#define GAME_STATUS_SIZE(GameState, width, height) (sizeof(GameState) + (sizeof(int) * (width * height)))

/* Macro para verificar si una posicion es la cabeza de un jugador. */
#define IS_HEAD(x, y, j, i) ((x) == (j) && (y) == (i))

/* Macro para acceder a una posicion del tablero. */
#define BOARD_AT(board, w, i, j) ((board)[((i) * (w)) + (j)])


/* Estructura que representa a un jugador. */
typedef struct {
    char name[NAME_DIM];                /* Nombre del jugador */
    unsigned int score;                 /* Puntaje acumulado */
    unsigned int validMoves;            /* Cantidad de movimientos validos */
    unsigned int invalidMoves;          /* Cantidad de movimientos invalidos */
    unsigned short x;                   /* Posicion X en el tablero */
    unsigned short y;                   /* Posicion Y en el tablero */
    pid_t pid;                          /* PID del proceso del jugador */
    bool isValid;                       /* true si el jugador tiene movimientos validos */
} Player;


/* Estructura que representa el estado del juego. */
typedef struct {
    unsigned short width;               /* Ancho del tablero */
    unsigned short height;              /* Alto del tablero */
    unsigned char playerCount;          /* Cantidad de jugadores en el juego */
    Player players[CANT_PLAYERS];       /* Informacion de los jugadores */
    bool gameOver;                      /* true si el juego ha terminado */
    int board[];                        /* Tablero flexible: fila-0, fila-1, ..., fila-n-1 */
} GameState;


/* Estructura que contiene los semaforos de sincronizacion. */
typedef struct {
    sem_t printNeeded;                  /* Indica a la vista que hay cambios por imprimir */
    sem_t renderDone;                   /* Indica al master que la vista termino de imprimir */
    sem_t masterMutex;                  /* Mutex para prioridad del master */
    sem_t stateMutex;                   /* Mutex para acceso exclusivo al estado */
    sem_t readCountMutex;               /* Mutex para el contador de lectores */
    unsigned int readersCount;          /* Cantidad de procesos leyendo el estado */
    sem_t playerSem[CANT_PLAYERS];      /* Semaforo por jugador para enviar movimiento */
} SyncData;


/* Estructura con los parametros de configuracion del juego. */
typedef struct {
    size_t width;                       /* Ancho del tablero */
    size_t height;                      /* Alto del tablero */
    size_t delay;                       /* Retraso en milisegundos */
    size_t timeout;                     /* Timeout en segundos */
    size_t seed;                        /* Semilla para generacion aleatoria */
    char *view;                         /* Ruta del binario de la vista */
    char *players[CANT_PLAYERS];        /* Rutas de los binarios de los jugadores */
    size_t playerCount;                 /* Cantidad de jugadores */
} Params;


/* Tipo para funciones manejadoras de parametros. */
typedef int (*ParamHandler)(const char *value, void *context);


/* Estructura que describe un parametro de linea de comandos. */
typedef struct {
    const char *flag;                   /* Flag del parametro (ej: "-w") */
    int expectsValue;                   /* 1 si espera un valor, 0 si no */
    ParamHandler handler;               /* Funcion manejadora */
    const char *help;                   /* Texto de ayuda */
} ParamDef;


/* Estructura que representa un proceso jugador. */
typedef struct {
    pid_t pid;                          /* PID del proceso */
    int pipeFd;                         /* File descriptor del pipe */
} PlayerProcess;

#endif
