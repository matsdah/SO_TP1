#include <stdio.h>
#include "../include/structures.h"
#include "../include/paramsHandler.h"
#include "../include/shmState.h"
#include "../include/shmSync.h"


int main(int argc, char *argv[]){

    /* paramsHandler */
    Parameters params = default_parameters();

    if(!parse_parameters(argc, argv, &params)){
        return 1;
    }

    printf("width: %zu\n", params.width);
    printf("height: %zu\n", params.height);

    /* SHM + Sync (mínimo para compilar y validar IPC) */
    int stateFd = -1;
    int syncFd = -1;
    GameState *gameState = NULL;
    semaphoresStatus *gameSync = NULL;

    if (stateCreate(&stateFd, &gameState, params.width, params.height) == -1) {
        perror("stateCreate");
        return 1;
    }
    if (syncCreate(&syncFd, &gameSync) == -1) {
        perror("syncCreate");
        (void)stateClose(stateFd, gameState, params.width, params.height);
        (void)stateUnlink();
        return 1;
    }
    if (syncInit(gameSync, (unsigned int)params.amount_players) == -1) {
        perror("syncInit");
        (void)syncClose(syncFd, gameSync);
        (void)syncUnlink();
        (void)stateClose(stateFd, gameState, params.width, params.height);
        (void)stateUnlink();
        return 1;
    }

    gameState->width = (unsigned short)params.width;
    gameState->height = (unsigned short)params.height;
    gameState->playersCount = (unsigned char)params.amount_players;
    gameState->gameOver = true; /* por ahora no hay loop de juego */

    (void)syncDestroy(gameSync, (unsigned int)params.amount_players);
    (void)syncClose(syncFd, gameSync);
    (void)syncUnlink();
    (void)stateClose(stateFd, gameState, params.width, params.height);
    (void)stateUnlink();
    

    return 0;
}
