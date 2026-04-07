#ifndef VIEW_RENDER_H
#define VIEW_RENDER_H

#include <structures.h>
#include <stddef.h>

/* Funciones para imprimir la vista del juego. */
void printView(const GameState *gameState);

/* Función para limpiar la pantalla. */
void clearScreen();

/* Función para imprimir las estadísticas del juego. */
void printStats(GameState * gameState);

/* Función para imprimir las estadísticas de un jugador. */
void printPlayerStats(Player* player_state);

#endif
