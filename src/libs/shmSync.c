#include <shmSync.h>

int syncCreate(int *shmFd, SyncData **gameSync){
    if(!gameSync || !shmFd){
        /* Parámetros inválidos. */
        fprintf(stderr, "Error: Parámetros inválidos en syncCreate.\n");
        return -1;
    }

    /* Crea y mapea la memoria compartida. */
    void *addr = shmCreateAndMap(GAME_SYNC_SHM_NAME, sizeof(SyncData), 0644, shmFd, PROT_READ | PROT_WRITE);

    if(addr == MAP_FAILED){
        /* Error al crear la memoria compartida. */
        fprintf(stderr, "Error: No se pudo crear la memoria compartida para sincronización.\n");
        return -1;
    }

    *gameSync = (SyncData *)addr;

    return 0;
}

int syncInit(SyncData *gameSync, unsigned int playerCount){
    if(!gameSync){
        /* Parámetros inválidos. */
        fprintf(stderr, "Error: Parámetros inválidos en syncInit.\n");

        return -1;
    }

    memset(gameSync, 0, sizeof(SyncData));      /* Inicializa la estructura a cero. */

    if(sem_init(&gameSync->printNeeded, 1, 0) == -1){
        /* Error al inicializar el semáforo de impresión. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo de impresión.\n");
        return -1;
    }

    if(sem_init(&gameSync->renderDone, 1, 0) == -1){
        /* Error al inicializar el semáforo de renderizado. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo de finalizacion de renderizado.\n");
        return -1;
    }

    if(sem_init(&gameSync->masterMutex, 1, 1) == -1){
        /* Error al inicializar el semáforo del master. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo del master.\n");
        return -1;
    }

    if(sem_init(&gameSync->stateMutex, 1, 1) == -1){
        /* Error al inicializar el semáforo del estado. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo del exclusión mutua del estado.\n");
        return -1;
    }

    if(sem_init(&gameSync->readCountMutex, 1, 1) == -1){
        /* Error al inicializar el semáforo de lectores. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo de cantidad de lectores.\n");
        return -1;
    }

    gameSync->readersCount = 0;     /* Inicializa el contador de lectores. */

    for(unsigned int i = 0; i < playerCount; i++){
        if(sem_init(&gameSync->playerSem[i], 1, 0) == -1){
            /* Error al inicializar el semáforo del jugador i. */
            fprintf(stderr, "Error: No se pudo inicializar el semáforo del jugador %u.\n", i);
            return -1;
        }
    }

    return 0;
}

int syncDestroy(SyncData *gameSync, unsigned int playerCount){
    if(!gameSync){
        fprintf(stderr, "Error: Parámetros inválidos en syncDestroy.\n");
        return -1;
    }

    sem_t *syncSemaphores[] = {
        &gameSync->printNeeded,
        &gameSync->renderDone,
        &gameSync->masterMutex,
        &gameSync->stateMutex,
        &gameSync->readCountMutex
    };

    const char *syncDestroyErrorMessages[] = {
        "Error: No se pudo destruir el semáforo de impresión.\n",
        "Error: No se pudo destruir el semáforo de finalizacion de renderizado.\n",
        "Error: No se pudo destruir el semáforo del master.\n",
        "Error: No se pudo destruir el semáforo del exclusión mutua del estado.\n",
        "Error: No se pudo destruir el semáforo de cantidad de lectores.\n"
    };

    unsigned int semCount = sizeof(syncSemaphores) / sizeof(syncSemaphores[0]);
    for(unsigned int i = 0; i < semCount; i++){
        if(sem_destroy(syncSemaphores[i]) == -1){
            fprintf(stderr, "%s", syncDestroyErrorMessages[i]);
            return -1;
        }
    }

    for(unsigned int i = 0; i < playerCount; i++){
        if(sem_destroy(&gameSync->playerSem[i]) == -1){
            fprintf(stderr, "Error: No se pudo destruir el semáforo del jugador %u.\n", i);
            return -1;
        }
    }

    return 0;
}

int syncUnlink(void){
    return shmRemoveFd(GAME_SYNC_SHM_NAME);
}

int syncOpen(int *shmFd, SyncData **gameSync){
    if(!gameSync || !shmFd){
        /* Parámetros inválidos. */
        fprintf(stderr, "Error: Parámetros inválidos en syncOpen.\n");
        return -1;
    }

    void *addr = shmOpenAndMap(GAME_SYNC_SHM_NAME, sizeof(SyncData), O_RDWR, shmFd, PROT_READ | PROT_WRITE);

    if(addr == MAP_FAILED){
        /* Error al abrir la memoria compartida. */
        fprintf(stderr, "Error: No se pudo abrir la memoria compartida.\n");
        return -1;
    }

    *gameSync = (SyncData *)addr;

    return 0;
}

int syncClose(int shmFd, SyncData *gameSync){
    if(!gameSync){
        fprintf(stderr, "Error: Parámetros inválidos en syncClose.\n");
        return -1;
    }

    int unmapResult = shmUnmapFd(gameSync, sizeof(SyncData));
    int closeResult = shmCloseFd(shmFd);

    if(unmapResult == -1){
        fprintf(stderr, "Error: No se pudo desmapear la memoria compartida de sincronización.\n");
    }
    if(closeResult == -1){
        fprintf(stderr, "Error: No se pudo cerrar el descriptor de memoria compartida de sincronización.\n");
    }

    return ((unmapResult == -1 || closeResult == -1) ? -1 : 0);
}

void acquireReadLock(SyncData *sync){

    sem_wait(&sync->masterMutex);

    sem_wait(&sync->readCountMutex);

    sync->readersCount++;

    if(sync->readersCount == 1){
        sem_wait(&sync->stateMutex);    /* Primer lector bloquea */
    }

    sem_post(&sync->readCountMutex);

    sem_post(&sync->masterMutex);
}

void releaseReadLock(SyncData *sync){

    sem_wait(&sync->readCountMutex);

    sync->readersCount--;

    if(sync->readersCount == 0){
        sem_post(&sync->stateMutex);    /* Ultimo lector desbloquea */
    }
    sem_post(&sync->readCountMutex);
}
