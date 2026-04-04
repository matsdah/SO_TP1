#include <stdio.h>
#include "../include/structures.h"
#include "../include/paramsHandler.h"
#include "../include/shmCommon.h"
#include "../include/shmState.h"
#include "../include/shmSync.h"
#include "../include/masterProcesses.h"


int main(int argc, char const *argv[]){    

    /* paramsHandler */
    Parameters params = default_parameters();

    if(!parse_parameters(argc, argv, &params)){
        return 1;
    }

    printf("width: %zu\n", params.width);
    printf("height: %zu\n", params.height);

    /* shm */
    GameState * gameState = createGameState(sizeof(GameState) + (sizeof(int)  *(params.height * params.width)));
    semaphoresStatus * gameSync = createGameSync();
    
    setInitialGameState(GameState * gameState, Parameters params);
    

    return 0;
}
