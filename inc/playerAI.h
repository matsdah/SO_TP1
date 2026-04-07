#ifndef PLAYER_AI_H
#define PLAYER_AI_H

#include <structures.h>

/* Encuentra el índice del jugador actual por PID. */
int find_my_index(GameState *state);

/* Adquiere el lock de lectura. */
void acquire_read_lock(semaphoresStatus *sync);

/* Libera el lock de lectura. */
void release_read_lock(semaphoresStatus *sync);

/* Encuentra la celda con mayor valor adyacente. */
unsigned char find_best_move(GameState *state, int my_index);

#endif
