#ifndef PLAYER_AI_H
#define PLAYER_AI_H

#include <structures.h>

/* Busca el indice del jugador actual comparando PIDs. */
int findMyIndex(GameState *state);

/* Calcula el mejor movimiento usando estrategia greedy (mayor valor adyacente). */
unsigned char findBestMove(GameState *state, int myIndex);

#endif
