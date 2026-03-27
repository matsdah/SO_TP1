#include "shmCommon.h"
#include "structures.h"

GameState * createGameState(size_t size){
    return (GameState *) createSHM("/gameState", size, 0644);
}

