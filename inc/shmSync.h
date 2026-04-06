#ifndef SHM_SYNC_H
#define SHM_SYNC_H

#include "structures.h"

#define GAME_SYNC_SHM_NAME "/game_sync"

/* Master: crea + mapea memoria compartida del semaforo */
int syncCreate(int * shmFd, semaphoresStatus ** gameSync);

/* Master: Inicializa semaforos y contador */
int syncInit(semaphoresStatus * gameSync, unsigned int playersCount);

/* Master: destruye semaforos */
int syncDestroy(semaphoresStatus * gameSync, unsigned int playersCount);

/* Master: elimina SHM */
int syncUnlink();

/* Vista/Jugador: abre + mapea memoria compartida del semaforo */
int syncOpen(int * shmFd, semaphoresStatus ** gameSync);

/* Cierra mapping + fd */
int syncClose(int shmFd, semaphoresStatus * gameSync);

#endif
