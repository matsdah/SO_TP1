#ifndef SHM_STATE_H
#define SHM_STATE_H

#include <stddef.h>
#include <structures.h>

#define GAME_STATE_SHM_NAME "/game_state"

/* Calcula el tamaño total de la memoria compartida del estado. */
size_t stateGetSize(size_t width, size_t height);

/* Crea y mapea la memoria compartida del estado del juego (master). */
int stateCreate(int *shmFd, GameState **gameState, size_t width, size_t height);

/* Abre y mapea la memoria compartida del estado existente (vista/jugador). */
int stateOpen(int *shmFd, GameState **gameState, size_t width, size_t height);

/* Cierra el mapping y el file descriptor del estado. */
int stateClose(int shmFd, GameState *gameState, size_t width, size_t height);

/* Elimina el objeto de memoria compartida del estado. */
int stateUnlink(void);

#endif
