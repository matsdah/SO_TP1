#ifndef SHM_SYNC_H
#define SHM_SYNC_H

#include <structures.h>

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

/* Adquiere lectura compartida. Devuelve 0 en éxito, -1 en error. */
int acquireReadLock(SyncData *sync);

/* Libera lectura compartida. Devuelve 0 en éxito, -1 en error. */
int releaseReadLock(SyncData *sync);

#endif
