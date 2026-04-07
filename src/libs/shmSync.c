#include <shmSync.h>
#include <shmCommon.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

int syncCreate(int *shmFd, SyncData **gameSync) {
    if (!gameSync || !shmFd) {
        return -1;
    }

    void *addr = shmCreateAndMap(GAME_SYNC_SHM_NAME, sizeof(SyncData), 
                                     0644, shmFd, PROT_READ | PROT_WRITE);
    if (addr == MAP_FAILED) {
        return -1;
    }

    *gameSync = (SyncData *)addr;
    return 0;
}

int syncInit(SyncData *gameSync, unsigned int playerCount) {
    if (!gameSync) {
        return -1;
    }
    memset(gameSync, 0, sizeof(SyncData));

    if (sem_init(&gameSync->printNeeded, 1, 0) == -1) {
        return -1;
    }
    if (sem_init(&gameSync->renderDone, 1, 0) == -1) {
        return -1;
    }
    if (sem_init(&gameSync->masterMutex, 1, 1) == -1) {
        return -1;
    }
    if (sem_init(&gameSync->stateMutex, 1, 1) == -1) {
        return -1;
    }
    if (sem_init(&gameSync->readCountMutex, 1, 1) == -1) {
        return -1;
    }

    gameSync->readersCount = 0;

    for (unsigned int i = 0; i < playerCount; i++) {
        if (sem_init(&gameSync->playerSem[i], 1, 0) == -1) {
            return -1;
        }
    }

    return 0;
}

int syncDestroy(SyncData *gameSync, unsigned int playerCount) {
    int result = 0;

    if (!gameSync) {
        return -1;
    }

    if (sem_destroy(&gameSync->printNeeded) == -1) {
        result = -1;
    }
    if (sem_destroy(&gameSync->renderDone) == -1) {
        result = -1;
    }
    if (sem_destroy(&gameSync->masterMutex) == -1) {
        result = -1;
    }
    if (sem_destroy(&gameSync->stateMutex) == -1) {
        result = -1;
    }
    if (sem_destroy(&gameSync->readCountMutex) == -1) {
        result = -1;
    }

    for (unsigned int i = 0; i < playerCount; i++) {
        if (sem_destroy(&gameSync->playerSem[i]) == -1) {
            result = -1;
        }
    }

    return result;
}

int syncUnlink(void) {
    return shmRemoveFd(GAME_SYNC_SHM_NAME);
}

int syncOpen(int *shmFd, SyncData **gameSync) {
    if (!gameSync || !shmFd) {
        return -1;
    }

    void *addr = shmOpenAndMap(GAME_SYNC_SHM_NAME, sizeof(SyncData), 
                                   O_RDWR, shmFd, PROT_READ | PROT_WRITE);
    if (addr == MAP_FAILED) {
        return -1;
    }

    *gameSync = (SyncData *)addr;
    return 0;
}

int syncClose(int shmFd, SyncData *gameSync) {
    if (!gameSync) {
        return -1;
    }

    return (shmUnmapFd(gameSync, sizeof(SyncData)) == -1 || 
            shmCloseFd(shmFd) == -1) ? -1 : 0;
}

void acquireReadLock(SyncData *sync) {
    sem_wait(&sync->masterMutex);       /* Respetar prioridad del escritor */
    sem_wait(&sync->readCountMutex);
    sync->readersCount++;
    if (sync->readersCount == 1) {
        sem_wait(&sync->stateMutex);    /* Primer lector bloquea */
    }
    sem_post(&sync->readCountMutex);
    sem_post(&sync->masterMutex);
}

void releaseReadLock(SyncData *sync) {
    sem_wait(&sync->readCountMutex);
    sync->readersCount--;
    if (sync->readersCount == 0) {
        sem_post(&sync->stateMutex);    /* Ultimo lector desbloquea */
    }
    sem_post(&sync->readCountMutex);
}
