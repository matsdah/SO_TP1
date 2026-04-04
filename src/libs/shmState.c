#include "shmCommon.h"
#include "shmState.h"
#include "structures.h"

size_t gameStateSize(size_t width, size_t height){
    return sizeof(GameState) + (sizeof(int) * (width * height));
}

int stateCreate(int * shmFd, GameState ** gameState, size_t width, size_t height){
    if(!gameState){
        return -1;
    }
    
    void * addr = createAndMap(GAME_STATE_SHM_NAME, gameStateSize(width, height), 0600, shmFd, PROT_READ | PROT_WRITE);
    return (addr == MAP_FAILED) ? -1 : 0;
}

int stateOpen(int * shmFd, GameState ** gameState, size_t width, size_t height){
    if(!gameState){
        return -1;
    }
    
    void * addr = openAndMap(GAME_STATE_SHM_NAME, gameStateSize(width, height), O_RDWR, shmFd, PROT_READ | PROT_WRITE);
    
    if(addr == MAP_FAILED){
        return -1;
    } else {
        *gameState = (GameState *) addr;
        return 0;
    }
}

int stateClose(int shmFd, GameState * gameState, size_t width, size_t height){
    return (!gameState && (unmapFd(gameState, gameStateSize(width, height)) == -1|| closeFd(shmFd) == -1)) ? -1 : 0;
}

int stateUnlink(){
    return removeFd(GAME_STATE_SHM_NAME);
}

// sixseven 67 67 67 67 67 67 six seve
// aguante la academia, rojo puto
// vamo acade 
// 
