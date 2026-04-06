#include "../../include/shmSync.h"

#include "../../include/shmCommon.h"

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

    if(sem_init(&gameSync->showNeeded, 1, 0) == -1){
        return -1;
    }
    if(sem_init(&gameSync->showDone, 1, 0) == -1){
        return -1;
    }

    if(sem_init(&gameSync->masterMutex, 1, 1) == -1){
        return -1;
    }
    if(sem_init(&gameSync->gameStateMutex, 1, 1) == -1){
        return -1;
    }
    if(sem_init(&gameSync-> nextVariableMutex, 1, 1) == -1){
        return -1;
    }

    gameSync ->playersReadingStatus = 0;

    for(int i = 0; i < playersCount; i++){
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

    if(sem_destroy(&gameSync->showNeeded) == -1){
        toReturn = -1;
    }
    if(sem_destroy(&gameSync->showDone) == -1){
        toReturn = -1;
    }
    if(sem_destroy(&gameSync->masterMutex) == -1){
        toReturn = -1;
    }
    if(sem_destroy(&gameSync->gameStateMutex) == -1){
        toReturn = -1;
    }
    if(sem_destroy(&gameSync->nextVariableMutex) == -1){
        toReturn = -1;
    }

    for(int i = 0; i < playersCount; i++){
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
