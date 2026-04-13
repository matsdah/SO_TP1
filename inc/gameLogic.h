#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <structures.h>
#include <time.h>
#include <signal.h>

/* Inicializa el tablero con valores aleatorios entre 1 y 9. */
void initializeBoard(GameState *state, unsigned int seed);

/* Posiciona a los jugadores en el tablero de forma distribuida. */
void placePlayers(GameState *state);

/* Crea los procesos de los jugadores y configura los pipes. */
int spawnPlayers(Params *params, PlayerProcess *processes, GameState *state);

/* Crea e inicializa memoria compartida y semáforos del juego. */
int setupGameResources(const Params *params, int *stateFd, GameState **state, int *syncFd, SyncData **sync);

/* Inicializa los campos del estado y el tablero del juego. */
void setupInitialGameState(GameState *state, const Params *params);

/* Inicializa estructura de procesos de jugadores. */
void initPlayerProcesses(PlayerProcess *processes, int count);

/* Crea proceso de vista (si corresponde) y actualiza su estado. */
int spawnViewProcess(const Params *params, pid_t *viewPid, int *viewReady);

/* Ejecuta el loop principal del master. */
void runMasterLoop(GameState *gameState, SyncData *gameSync, PlayerProcess *playerProcesses, const Params *params, int *viewReady, volatile sig_atomic_t *interrupted);

/* Espera finalización del proceso de vista e imprime su estado. */
int waitViewProcess(pid_t viewPid);

/* Valida si un movimiento en la direccion dada es legal. */
int validateMove(GameState *state, int playerIdx, unsigned char direction);

/* Aplica un movimiento valido actualizando el estado del juego. */
void applyMove(GameState *state, int playerIdx, unsigned char direction);

/* Notifica a la vista que debe renderizar el estado actual. */
int notifyView(SyncData *sync);

/* Verifica si el juego ha terminado por timeout o bloqueo de todos los jugadores. */
void checkGameOver(GameState *state, time_t startTime, size_t timeout);

/* Imprime los resultados finales del juego. */
void printResults(GameState *state);

/* Limpia todos los recursos: procesos, memoria compartida y semaforos. */
void cleanup(PlayerProcess *processes, int count, GameState *state, SyncData *sync, int stateFd, int syncFd, size_t width, size_t height, pid_t viewPid, int interrupted);

#endif