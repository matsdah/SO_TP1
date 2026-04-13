#ifndef VIEW_RENDER_H
#define VIEW_RENDER_H

#include <structures.h>

/* Renderiza el tablero completo con colores por jugador. */
void printView(const GameState *gameState);

/* Limpia la pantalla de la terminal. */
void clearScreen(void);

/* Entra al buffer alternativo de pantalla. */
void enterAlternateScreen(void);

/* Sale del buffer alternativo de pantalla. */
void leaveAlternateScreen(void);

/* Imprime las estadisticas de todos los jugadores. */
void printStats(const GameState *gameState);

/* Imprime las estadisticas de un jugador individual. */
void printPlayerStats(const Player *playerState);

#endif
