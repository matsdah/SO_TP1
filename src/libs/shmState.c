#include <shmState.h>
#include <shmCommon.h>
#include <fcntl.h>
#include <sys/mman.h>

size_t stateGetSize(size_t width, size_t height) {
    return sizeof(GameState) + (sizeof(int) * (width * height));
}

int stateCreate(int *shmFd, GameState **gameState, size_t width, size_t height) {
    if (!gameState || !shmFd) {
        return -1;
    }

    void *addr = shmCreateAndMap(GAME_STATE_SHM_NAME, stateGetSize(width, height), 
                                     0644, shmFd, PROT_READ | PROT_WRITE);
    if (addr == MAP_FAILED) {
        return -1;
    }

    *gameState = (GameState *)addr;
    return 0;
}

int stateOpen(int *shmFd, GameState **gameState, size_t width, size_t height) {
    if (!gameState || !shmFd) {
        return -1;
    }

    void *addr = shmOpenAndMap(GAME_STATE_SHM_NAME, stateGetSize(width, height), 
                                   O_RDONLY, shmFd, PROT_READ);
    if (addr == MAP_FAILED) {
        return -1;
    }

    *gameState = (GameState *)addr;
    return 0;
}

int stateClose(int shmFd, GameState *gameState, size_t width, size_t height) {
    if (!gameState) {
        return -1;
    }

    return (shmUnmapFd(gameState, stateGetSize(width, height)) == -1 || 
            shmCloseFd(shmFd) == -1) ? -1 : 0;
}

int stateUnlink(void) {
    return shmRemoveFd(GAME_STATE_SHM_NAME);
}
