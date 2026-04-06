#ifndef SHMSTATE_H
#define SHMSTATE_H

#include <stddef.h>
#include <structures.h>

#define GAME_STATE_SHM_NAME "/game_state"

/* Tamaño total del SHM state */
size_t gameStateSize(size_t width, size_t height);

/* Master: Crea y mapea el SHM state */
int stateCreate(int * shmFd, GameState ** gameState, size_t width, size_t height);

/* Vista/Jugador: abre + mapea el SHM state */
int stateOpen(int * shmFd, GameState ** gameState, size_t width, size_t height);

/* Cierra mapping + fd */
int stateClose(int shmFd, GameState * gameState, size_t width, size_t height);

/* Elimina el SHM state */
int stateUnlink();

#endif