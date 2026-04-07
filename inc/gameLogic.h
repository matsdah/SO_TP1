#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <structures.h>
#include <time.h>
#include <shmState.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <shmSync.h>

/* Initializa el tablero con valores random entre 1 y 9. */
void initializeBoard(GameState *state, unsigned int seed);

/* Posiciona a los jugadores en el tablero. */
void placePlayers(GameState *state);

/* Procesos de jugadores con pipes. */
int spawnPlayers(Parameters *params, PlayerProcess *processes, GameState *state);

/* Valida si un movimiento es legal. */
int validateMove(GameState *state, int playerIdx, unsigned char direction);

/* Aplica un movimiento válido. */
void applyMove(GameState *state, int playerIdx, unsigned char direction);

/* Notify view to render */
void notifyView(semaphoresStatus *sync);

/* Chequea si el juego ha terminado. */
void checkGameOver(GameState *state, time_t startTime, size_t timeout);

/* Print final results */
void printResults(GameState *state);

/* Cleanup resources */
void cleanup(PlayerProcess *processes, int count, GameState *state, semaphoresStatus *sync, int stateFd, int syncFd, size_t width, size_t height, pid_t viewPid);

#endif
