#include <shmState.h>

size_t stateGetSize(size_t width, size_t height){
    return (sizeof(GameState) + (sizeof(int) * (width * height)));
}

int stateCreate(int *shmFd, GameState **gameState, size_t width, size_t height) {
    if(!gameState || !shmFd){
        return -1;
    }

    /* Crea y mapea la memoria compartida. */
    void *addr = shmCreateAndMap(GAME_STATE_SHM_NAME, stateGetSize(width, height),0644, shmFd, PROT_READ | PROT_WRITE);

    if(addr == MAP_FAILED){
        /* Error al crear la memoria compartida. */
        return -1;
    }

    *gameState = (GameState *)addr;

    return 0;
}

int stateOpen(int *shmFd, GameState **gameState, size_t width, size_t height){
    if(!gameState || !shmFd){
        /* Parámetros inválidos. */
        return -1;
    }

    /* Abre y mapea la memoria compartida. */
    void *addr = shmOpenAndMap(GAME_STATE_SHM_NAME, stateGetSize(width, height), O_RDONLY, shmFd, PROT_READ);

    if(addr == MAP_FAILED){
        /* Error al abrir la memoria compartida. */
        return -1;
    }

    *gameState = (GameState *)addr;

    return 0;
}

int stateClose(int shmFd, GameState *gameState, size_t width, size_t height){
    if(!gameState){
        /* Parámetros inválidos. */
        return -1;
    }

    return ((shmUnmapFd(gameState, stateGetSize(width, height)) == -1) || ((shmCloseFd(shmFd) == -1) ? -1 : 0));
}

int stateUnlink(){
    return shmRemoveFd(GAME_STATE_SHM_NAME);
}
