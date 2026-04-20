#include <shmState.h>
#include <shmCommon.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>

/* 
** shmState.c -> crea/abre/cierra/destruye SHM del estado y calcula su tamaño total. 
*/

size_t stateGetSize(size_t width, size_t height){
    return (sizeof(GameState) + (sizeof(char) * (width * height)));
}

int stateCreate(int *shmFd, GameState **gameState, size_t width, size_t height) {
    if(!gameState || !shmFd){
        /* Parámetros inválidos. */
        fprintf(stderr, "Error: Parámetros inválidos en stateCreate.\n");
        return -1;
    }

    /* Crea y mapea la memoria compartida. */
    void *addr = shmCreateAndMap(GAME_STATE_SHM_NAME, stateGetSize(width, height),0644, shmFd, PROT_READ | PROT_WRITE);

    if(addr == MAP_FAILED){
        /* Error al crear la memoria compartida. */
        fprintf(stderr, "Error: No se pudo crear la memoria compartida para el estado del juego.\n");
        return -1;
    }

    *gameState = (GameState *)addr;

    return 0;
}

int stateOpen(int *shmFd, GameState **gameState, size_t width, size_t height){
    if(!gameState || !shmFd){
        /* Parámetros inválidos. */
        fprintf(stderr, "Error: Parámetros inválidos en stateOpen.\n");
        return -1;
    }

    /* Abre y mapea la memoria compartida. */
    void *addr = shmOpenAndMap(GAME_STATE_SHM_NAME, stateGetSize(width, height), O_RDONLY, shmFd, PROT_READ);

    if(addr == MAP_FAILED){
        /* Error al abrir la memoria compartida. */
        fprintf(stderr, "Error: No se pudo abrir la memoria compartida para el estado del juego.\n");
        return -1;
    }

    *gameState = (GameState *)addr;

    return 0;
}

int stateClose(int shmFd, GameState *gameState, size_t width, size_t height){
    if(!gameState){
        /* Parámetros inválidos. */
        fprintf(stderr, "Error: Parámetros inválidos en stateClose.\n");
        return -1;
    }

    int unmapResult = shmUnmapFd(gameState, stateGetSize(width, height));
    int closeResult = shmCloseFd(shmFd);

    if(unmapResult == -1){
        fprintf(stderr, "Error: No se pudo desmapear la memoria compartida del estado del juego.\n");
    }

    if(closeResult == -1){
        fprintf(stderr, "Error: No se pudo cerrar el descriptor de memoria compartida del estado del juego.\n");
    }

    return ((unmapResult == -1 || closeResult == -1) ? -1 : 0);
}

int stateUnlink(){
    return shmRemoveFd(GAME_STATE_SHM_NAME);
}
