#include <shmSync.h>
#include <shmCommon.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

int syncCreate(int * shmFd, semaphoresStatus ** gameSync){
    if(!gameSync || !shmFd){
        return -1;
    }

    void * addr = createAndMap(GAME_SYNC_SHM_NAME, sizeof(semaphoresStatus), 0644, shmFd, PROT_READ | PROT_WRITE);
    if(addr == MAP_FAILED){
        return -1;
    }

    *gameSync = (semaphoresStatus *)addr;
    return 0;
}

int syncInit(semaphoresStatus * gameSync, unsigned int playersCount){
    if(!gameSync){
        return -1;
    }
    memset(gameSync, 0, sizeof(semaphoresStatus));

    if(sem_init(&gameSync->printNeeded, 1, 0) == -1){
        return -1;
    }
    if(sem_init(&gameSync->renderDone, 1, 0) == -1){
        return -1;
    }

    if(sem_init(&gameSync->masterMutex, 1, 1) == -1){
        return -1;
    }
    if(sem_init(&gameSync->gameStateMutex, 1, 1) == -1){
        return -1;
    }
    if(sem_init(&gameSync->readCountMutex, 1, 1) == -1){
        return -1;
    }

    gameSync ->playersReadingStatus = 0;

    for(unsigned int i = 0; i < playersCount; i++){
        if(sem_init(&gameSync->playersAllowedToMove[i], 1, 0) == -1){
            return -1;
        }
    }

    return 0;
}

int syncDestroy(semaphoresStatus * gameSync, unsigned int playersCount){
    int toReturn = 0;
    
    if(!gameSync){
        return -1;
    }

    if(sem_destroy(&gameSync->printNeeded) == -1){
        toReturn = -1;
    }
    if(sem_destroy(&gameSync->renderDone) == -1){
        toReturn = -1;
    }
    if(sem_destroy(&gameSync->masterMutex) == -1){
        toReturn = -1;
    }
    if(sem_destroy(&gameSync->gameStateMutex) == -1){
        toReturn = -1;
    }
    if(sem_destroy(&gameSync->readCountMutex) == -1){
        toReturn = -1;
    }

    for(unsigned int i = 0; i < playersCount; i++){
        if(sem_destroy(&gameSync->playersAllowedToMove[i]) == -1){
            toReturn = -1;
        }
    }

    return toReturn;
}

int syncUnlink(){
    return removeFd(GAME_SYNC_SHM_NAME);
}

int syncOpen(int * shmFd, semaphoresStatus ** gameSync){
    if(!gameSync || !shmFd){
        return -1;
    }

    void * addr = openAndMap(GAME_SYNC_SHM_NAME, sizeof(semaphoresStatus), O_RDWR, shmFd, PROT_READ | PROT_WRITE);
    if(addr == MAP_FAILED){
        return -1;
    }

    *gameSync = (semaphoresStatus *)addr;
    return 0;
}

int syncClose(int shmFd, semaphoresStatus * gameSync){
    if(!gameSync){
        return -1;
    }

    return unmapFd(gameSync, sizeof(semaphoresStatus)) == -1 || closeFd(shmFd) == -1 ? -1 : 0;
}

void acquireReadLock(semaphoresStatus *sync){
    sem_wait(&sync->masterMutex);       /* Respetar prioridad del escritor */
    sem_wait(&sync->readCountMutex);
    sync->playersReadingStatus++;
    if (sync->playersReadingStatus == 1) {
        sem_wait(&sync->gameStateMutex);  /* Primer lector bloquea */
    }
    sem_post(&sync->readCountMutex);
    sem_post(&sync->masterMutex);
}

void releaseReadLock(semaphoresStatus *sync){
    sem_wait(&sync->readCountMutex);
    sync->playersReadingStatus--;
    if (sync->playersReadingStatus == 0) {
        sem_post(&sync->gameStateMutex);  /* Último lector desbloquea */
    }
    sem_post(&sync->readCountMutex);
}
