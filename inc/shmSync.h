#ifndef SHM_SYNC_H
#define SHM_SYNC_H

#include <structures.h>
#include <shmCommon.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>

#define GAME_SYNC_SHM_NAME "/game_sync"

/* Crea y mapea la memoria compartida de sincronizacion (master). */
int syncCreate(int *shmFd, SyncData **gameSync);

/* Inicializa los semaforos y contadores de sincronizacion (master). */
int syncInit(SyncData *gameSync, unsigned int playerCount);

/* Destruye los semaforos de sincronizacion (master). */
int syncDestroy(SyncData *gameSync, unsigned int playerCount);

/* Elimina el objeto de memoria compartida de sincronizacion. */
int syncUnlink();

/* Abre y mapea la memoria compartida de sincronizacion (vista/jugador). */
int syncOpen(int *shmFd, SyncData **gameSync);

/* Cierra el mapping y el file descriptor de sincronizacion. */
int syncClose(int shmFd, SyncData *gameSync);

/* Adquiere lectura exclusiva. */
void acquireReadLock(SyncData *sync);

/* Libera lectura exclusiva. */
void releaseReadLock(SyncData *sync);

#endif