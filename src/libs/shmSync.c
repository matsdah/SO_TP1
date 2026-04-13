#include <shmSync.h>
#include <shmCommon.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>

/* 
** shmSync.c -> crea/abre/cierra SHM de sync, inicializa/destroza semáforos y 
** maneja exclusividad de lectura. 
*/

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

    if(playerCount > CANT_PLAYERS){
        fprintf(stderr, "Error: cantidad de jugadores inválida en syncInit.\n");
        return -1;
    }

    unsigned int initializedPlayers = 0;
    int printNeededInit = 0;
    int renderDoneInit = 0;
    int masterMutexInit = 0;
    int stateMutexInit = 0;
    int readCountMutexInit = 0;

    if(sem_init(&gameSync->printNeeded, 1, 0) == -1){
        /* Error al inicializar el semáforo de impresión. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo de impresión.\n");
        goto init_error;
    }
    printNeededInit = 1;

    if(sem_init(&gameSync->renderDone, 1, 0) == -1){
        /* Error al inicializar el semáforo de renderizado. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo de finalizacion de renderizado.\n");
        goto init_error;
    }
    renderDoneInit = 1;

    if(sem_init(&gameSync->masterMutex, 1, 1) == -1){
        /* Error al inicializar el semáforo del master. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo del master.\n");
        goto init_error;
    }
    masterMutexInit = 1;

    if(sem_init(&gameSync->stateMutex, 1, 1) == -1){
        /* Error al inicializar el semáforo del estado. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo del exclusión mutua del estado.\n");
        goto init_error;
    }
    stateMutexInit = 1;

    if(sem_init(&gameSync->readCountMutex, 1, 1) == -1){
        /* Error al inicializar el semáforo de lectores. */
        fprintf(stderr, "Error: No se pudo inicializar el semáforo de cantidad de lectores.\n");
        goto init_error;
    }
    readCountMutexInit = 1;

    gameSync->readersCount = 0;     /* Inicializa el contador de lectores. */

    for(unsigned int i = 0; i < playerCount; i++){
        if(sem_init(&gameSync->playerSem[i], 1, 0) == -1){
            /* Error al inicializar el semáforo del jugador i. */
            fprintf(stderr, "Error: No se pudo inicializar el semáforo del jugador %u.\n", i);
            goto init_error;
        }
        initializedPlayers++;
    }

    return 0;

init_error:
    for(unsigned int i = 0; i < initializedPlayers; i++){
        sem_destroy(&gameSync->playerSem[i]);
    }
    if(readCountMutexInit){
        sem_destroy(&gameSync->readCountMutex);
    }
    if(stateMutexInit){
        sem_destroy(&gameSync->stateMutex);
    }
    if(masterMutexInit){
        sem_destroy(&gameSync->masterMutex);
    }
    if(renderDoneInit){
        sem_destroy(&gameSync->renderDone);
    }
    if(printNeededInit){
        sem_destroy(&gameSync->printNeeded);
    }

    return -1;
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
    int hasErrors = 0;

    for(unsigned int i = 0; i < semCount; i++){
        if(sem_destroy(syncSemaphores[i]) == -1){
            fprintf(stderr, "%s", syncDestroyErrorMessages[i]);
            hasErrors = 1;
        }
    }

    if(playerCount > CANT_PLAYERS){
        playerCount = CANT_PLAYERS;
    }

    for(unsigned int i = 0; i < playerCount; i++){
        if(sem_destroy(&gameSync->playerSem[i]) == -1){
            fprintf(stderr, "Error: No se pudo destruir el semáforo del jugador %u.\n", i);
            hasErrors = 1;
        }
    }

    return (hasErrors ? -1 : 0);
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

int acquireReadLock(SyncData *sync){
    if(sync == NULL){
        fprintf(stderr, "Error: sync nulo en acquireReadLock.\n");
        return -1;
    }

    if(sem_wait(&sync->masterMutex) == -1){
        perror("Error en sem_wait masterMutex");
        return -1;
    }

    if(sem_wait(&sync->readCountMutex) == -1){
        perror("Error en sem_wait readCountMutex");
        if(sem_post(&sync->masterMutex) == -1){
            perror("Error en sem_post masterMutex");
        }
        return -1;
    }

    sync->readersCount++;

    if(sync->readersCount == 1){
        if(sem_wait(&sync->stateMutex) == -1){    /* Primer lector bloquea */
            perror("Error en sem_wait stateMutex");
            sync->readersCount--;
            if(sem_post(&sync->readCountMutex) == -1){
                perror("Error en sem_post readCountMutex");
            }
            if(sem_post(&sync->masterMutex) == -1){
                perror("Error en sem_post masterMutex");
            }
            return -1;
        }
    }

    if(sem_post(&sync->readCountMutex) == -1){
        perror("Error en sem_post readCountMutex");
        if(sem_post(&sync->masterMutex) == -1){
            perror("Error en sem_post masterMutex");
        }
        return -1;
    }

    if(sem_post(&sync->masterMutex) == -1){
        perror("Error en sem_post masterMutex");
        return -1;
    }

    return 0;
}

int releaseReadLock(SyncData *sync){
    if(sync == NULL){
        fprintf(stderr, "Error: sync nulo en releaseReadLock.\n");
        return -1;
    }

    if(sem_wait(&sync->readCountMutex) == -1){
        perror("Error en sem_wait readCountMutex");
        return -1;
    }

    sync->readersCount--;

    if(sync->readersCount == 0){
        if(sem_post(&sync->stateMutex) == -1){    /* Ultimo lector desbloquea */
            perror("Error en sem_post stateMutex");
            if(sem_post(&sync->readCountMutex) == -1){
                perror("Error en sem_post readCountMutex");
            }
            return -1;
        }
    }
    if(sem_post(&sync->readCountMutex) == -1){
        perror("Error en sem_post readCountMutex.");
        return -1;
    }

    return 0;
}
