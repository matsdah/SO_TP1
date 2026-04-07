#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>
#include <structures.h>
#include <paramsHandler.h>
#include <shmState.h>
#include <shmSync.h>
#include <gameLogic.h>

int main(int argc, char *argv[]){
    Parameters params = default_parameters();

    if(!parse_parameters(argc, argv, &params)){
        return 1;
    }

    /* Create shared memory and semaphores */
    int stateFd = -1;
    int syncFd = -1;
    GameState *gameState = NULL;
    semaphoresStatus *gameSync = NULL;

    if(stateCreate(&stateFd, &gameState, params.width, params.height) == -1){
        perror("stateCreate");
        return 1;
    }

    if(syncCreate(&syncFd, &gameSync) == -1){
        perror("syncCreate");
        
        stateClose(stateFd, gameState, params.width, params.height);
        stateUnlink();

        return 1;
    }

    if(syncInit(gameSync, (unsigned int)params.amount_players) == -1){
        perror("syncInit");

        syncClose(syncFd, gameSync);
        syncUnlink();
        stateClose(stateFd, gameState, params.width, params.height);
        stateUnlink();

        return 1;
    }

    /* Inicializa el estado del juego */
    gameState->width = params.width;
    gameState->height = params.height;
    gameState->playersCount = params.amount_players;
    gameState->gameOver = false;

    /* Initializa tablero. */
    initializeBoard(gameState, params.seed);

    /* Posiciona jugadores en el tablero. */
    placePlayers(gameState);

    /* Crea los procesos de los jugadores. */
    PlayerProcess playerProcesses[CANT_PLAYERS];

    if(spawnPlayers(&params, playerProcesses, gameState) == -1){

        cleanup(playerProcesses, params.amount_players, gameState, gameSync, stateFd, syncFd, params.width, params.height);

        return 1;
    }

    /* Crea el proceso de visualización. */
    pid_t viewPid = -1;

    if(params.view != NULL){
        viewPid = fork();

        if(viewPid == 0){
            char widthStr[16]; 
            char heightStr[16];

            snprintf(widthStr, sizeof(widthStr), "%zu", params.width);
            snprintf(heightStr, sizeof(heightStr), "%zu", params.height);

            execl(params.view, params.view, widthStr, heightStr, NULL);

            perror("execl view");
            exit(1);

        }else{
            if(viewPid < 0){
                perror("Error en Fork.");
            }
        }
    }

    /* Notifica el estado inicial a la vista. */
    if(viewPid > 0){
        notifyView(gameSync);
    }

    /* Main loop. */
    time_t startTime = time(NULL);
    int currentPlayer = 0;
    fd_set readFds;
    struct timeval tv;

    while(!gameState->gameOver){
        /* Check timeout y condiciones de gameover. */
        checkGameOver(gameState, startTime, params.timeout);

        if(gameState->gameOver){
            /* Salgo del loop. */
            break;
        }

        /* Round-robin: try to read from current player */
        FD_ZERO(&readFds);
        FD_SET(playerProcesses[currentPlayer].pipeFd, &readFds);

        /* Use delay as timeout for select */
        tv.tv_sec = 0;
        tv.tv_usec = params.delay * 1000;

        int ret = select(playerProcesses[currentPlayer].pipeFd + 1, &readFds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(playerProcesses[currentPlayer].pipeFd, &readFds)) {
            /* Read movement from player */
            unsigned char direction;
            ssize_t bytesRead = read(playerProcesses[currentPlayer].pipeFd, &direction, 1);

            if (bytesRead == 1) {
                /* Acquire writer lock */
                sem_wait(&gameSync->masterMutex);
                sem_wait(&gameSync->gameStateMutex);

                /* Validate and apply move */
                if (validateMove(gameState, currentPlayer, direction)) {
                    applyMove(gameState, currentPlayer, direction);
                    gameState->players[currentPlayer].validMovements++;
                    startTime = time(NULL); /* Reset timeout on valid move */
                } else {
                    gameState->players[currentPlayer].invalidMovements++;
                }

                /* Release writer lock */
                sem_post(&gameSync->gameStateMutex);
                sem_post(&gameSync->masterMutex);

                /* Notify view */
                if (viewPid > 0) {
                    notifyView(gameSync);
                }

                /* Send ACK to player */
                sem_post(&gameSync->playersAllowedToMove[currentPlayer]);
            }
        }

        /* Move to next player (round-robin) */
        currentPlayer = (currentPlayer + 1) % (int)params.amount_players;

        /* Small delay between iterations */
        usleep(params.delay * 1000);
    }

    /* Print final results */
    printResults(gameState);

    /* Cleanup */
    cleanup(playerProcesses, (int)params.amount_players, gameState, gameSync, stateFd, syncFd, params.width, params.height);

    /* Wait for view to finish */
    if (viewPid > 0) {
        waitpid(viewPid, NULL, 0);
    }

    return 0;
}
