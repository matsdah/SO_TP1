#ifndef VIEW_RENDER_H
#define VIEW_RENDER_H

#include <structures.h>
#include <stddef.h>

/* Renderiza el tablero completo con colores por jugador. */
void printView(const GameState *gameState);

/* Limpia la pantalla de la terminal. */
void clearScreen(void);

/* Imprime las estadisticas de todos los jugadores. */
void printStats(GameState *gameState);

/* Imprime las estadisticas de un jugador individual. */
void printPlayerStats(Player *playerState);

#endif
