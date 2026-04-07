#ifndef SHM_SYNC_H
#define SHM_SYNC_H

#include <structures.h>

#define GAME_SYNC_SHM_NAME "/game_sync"

/* Crea y mapea la memoria compartida de sincronizacion (solo master). */
int syncCreate(int *shmFd, SyncData **gameSync);

/* Inicializa los semaforos y contadores de sincronizacion (solo master). */
int syncInit(SyncData *gameSync, unsigned int playerCount);

/* Destruye los semaforos de sincronizacion (solo master). */
int syncDestroy(SyncData *gameSync, unsigned int playerCount);

/* Elimina el objeto de memoria compartida de sincronizacion. */
int syncUnlink(void);

/* Abre y mapea la memoria compartida de sincronizacion (vista/jugador). */
int syncOpen(int *shmFd, SyncData **gameSync);

/* Cierra el mapping y el file descriptor de sincronizacion. */
int syncClose(int shmFd, SyncData *gameSync);

/* Adquiere el lock de lectura (patron readers-writers). */
void acquireReadLock(SyncData *sync);

/* Libera el lock de lectura (patron readers-writers). */
void releaseReadLock(SyncData *sync);

#endif
