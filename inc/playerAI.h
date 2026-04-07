#ifndef PLAYER_AI_H
#define PLAYER_AI_H

#include <structures.h>

/* Encuentra el índice del jugador actual por PID. */
int find_my_index(GameState *state);

/* Encuentra la celda con mayor valor adyacente. */
unsigned char find_best_move(GameState *state, int my_index);

#endif
