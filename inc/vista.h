#ifndef VISTA_H
#define VISTA_H

#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <structures.h>

/* Funciones para imprimir la vista del juego. */
void printView(int * board,size_t height,size_t width);

/* Función para limpiar la pantalla. */
void clearScreen();

/* Función para imprimir las estadísticas del juego. */
void printStats(GameState * gameState);

/* Función para imprimir las estadísticas de un jugador. */
void printPlayerStats(Player* player_state);

#endif