#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <structures.h>
#include <paramsHandler.h>
#include <shmState.h>
#include <shmSync.h>

/* Direction vectors (0=UP, 1=UP_RIGHT, ..., 7=UP_LEFT) */
static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

/* Player process information */
typedef struct {
    pid_t pid;
    int pipeFd;
} PlayerProcess;

/* Function prototypes */
static void initializeBoard(GameState *state, unsigned int seed);
static void placePlayers(GameState *state);
static int spawnPlayers(Parameters *params, PlayerProcess *processes, GameState *state);
static int validateMove(GameState *state, int playerIdx, unsigned char direction);
static void applyMove(GameState *state, int playerIdx, unsigned char direction);
static void notifyView(semaphoresStatus *sync);
static void checkGameOver(GameState *state, time_t startTime, size_t timeout);
static void printResults(GameState *state);
static void cleanup(PlayerProcess *processes, int count, GameState *state, semaphoresStatus *sync, int stateFd, int syncFd, size_t width, size_t height);

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

    /* Initialize game state */
    gameState->width = (unsigned short)params.width;
    gameState->height = (unsigned short)params.height;
    gameState->playersCount = (unsigned char)params.amount_players;
    gameState->gameOver = false;

    /* Initialize board and place players */
    initializeBoard(gameState, (unsigned int)params.seed);
    placePlayers(gameState);

    /* Spawn player processes */
    PlayerProcess playerProcesses[CANT_PLAYERS];
    if (spawnPlayers(&params, playerProcesses, gameState) == -1) {
        cleanup(playerProcesses, (int)params.amount_players, gameState, gameSync, stateFd, syncFd, params.width, params.height);
        return 1;
    }

    /* Spawn view process if provided */
    pid_t viewPid = -1;
    if (params.view != NULL) {
        viewPid = fork();
        if (viewPid == 0) {
            char widthStr[16], heightStr[16];
            snprintf(widthStr, sizeof(widthStr), "%zu", params.width);
            snprintf(heightStr, sizeof(heightStr), "%zu", params.height);
            execl(params.view, params.view, widthStr, heightStr, NULL);
            perror("execl view");
            exit(1);
        } else if (viewPid < 0) {
            perror("fork view");
        }
    }

    /* Notify view of initial state */
    if (viewPid > 0) {
        notifyView(gameSync);
    }

    /* Main game loop */
    time_t startTime = time(NULL);
    int currentPlayer = 0;
    fd_set readFds;
    struct timeval tv;

    while (!gameState->gameOver) {
        /* Check timeout and game over conditions */
        checkGameOver(gameState, startTime, params.timeout);
        if (gameState->gameOver) {
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

/* Initialize board with random values 1-9 */
static void initializeBoard(GameState *state, unsigned int seed) {
    srand(seed);
    size_t totalCells = (size_t)state->width * state->height;
    for (size_t i = 0; i < totalCells; i++) {
        state->board[i] = (rand() % 9) + 1;
    }
}

/* Place players on board deterministically */
static void placePlayers(GameState *state) {
    unsigned short w = state->width;
    unsigned short h = state->height;
    unsigned char count = state->playersCount;

    /* Distribute players evenly across the board */
    for (unsigned char i = 0; i < count; i++) {
        state->players[i].x = (unsigned short)((i * w) / count);
        state->players[i].y = (unsigned short)((i * h) / count);
        state->players[i].playerScore = 0;
        state->players[i].validMovements = 0;
        state->players[i].invalidMovements = 0;
        state->players[i].valid = true;
        snprintf(state->players[i].playerName, NAME_DIM, "Player %d", i + 1);
        
        /* Mark initial position as occupied */
        int *cell = &BOARD_AT(state->board, w, state->players[i].y, state->players[i].x);
        state->players[i].playerScore += *cell;
        *cell = -(int)(i + 1);
    }
}

/* Spawn player processes with pipes */
static int spawnPlayers(Parameters *params, PlayerProcess *processes, GameState *state) {
    for (size_t i = 0; i < params->amount_players; i++) {
        int pipeFds[2];
        if (pipe(pipeFds) == -1) {
            perror("pipe");
            return -1;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return -1;
        }

        if (pid == 0) {
            /* Child process */
            close(pipeFds[0]); /* Close read end */
            
            /* Redirect stdout to pipe write end */
            if (dup2(pipeFds[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(pipeFds[1]);

            /* Execute player */
            char widthStr[16], heightStr[16];
            snprintf(widthStr, sizeof(widthStr), "%u", state->width);
            snprintf(heightStr, sizeof(heightStr), "%u", state->height);
            execl(params->players[i], params->players[i], widthStr, heightStr, NULL);
            perror("execl player");
            exit(1);
        } else {
            /* Parent process */
            close(pipeFds[1]); /* Close write end */
            processes[i].pid = pid;
            processes[i].pipeFd = pipeFds[0];
            state->players[i].playerPID = pid;
        }
    }
    return 0;
}

/* Validate if a move is legal */
static int validateMove(GameState *state, int playerIdx, unsigned char direction) {
    if (direction >= 8) {
        return 0; /* Invalid direction */
    }

    Player *player = &state->players[playerIdx];
    int newX = (int)player->x + DX[direction];
    int newY = (int)player->y + DY[direction];

    /* Check bounds */
    if (newX < 0 || newX >= (int)state->width || newY < 0 || newY >= (int)state->height) {
        return 0;
    }

    /* Check if cell is already occupied */
    int cellValue = BOARD_AT(state->board, state->width, newY, newX);
    if (cellValue < 0) {
        return 0; /* Cell is occupied */
    }

    return 1;
}

/* Apply a valid move */
static void applyMove(GameState *state, int playerIdx, unsigned char direction) {
    Player *player = &state->players[playerIdx];
    int newX = (int)player->x + DX[direction];
    int newY = (int)player->y + DY[direction];

    /* Get cell value and add to score */
    int *cell = &BOARD_AT(state->board, state->width, newY, newX);
    player->playerScore += (unsigned int)*cell;

    /* Mark cell as occupied */
    *cell = -(playerIdx + 1);

    /* Update player position */
    player->x = (unsigned short)newX;
    player->y = (unsigned short)newY;
}

/* Notify view to render */
static void notifyView(semaphoresStatus *sync) {
    sem_post(&sync->printNeeded);
    sem_wait(&sync->renderDone);
}

/* Check if game is over */
static void checkGameOver(GameState *state, time_t startTime, size_t timeout) {
    /* Check timeout */
    if (timeout > 0 && (time(NULL) - startTime) >= (time_t)timeout) {
        state->gameOver = true;
        return;
    }

    /* Check if all players are blocked */
    int activeCount = 0;
    for (unsigned char i = 0; i < state->playersCount; i++) {
        Player *player = &state->players[i];
        int hasValidMove = 0;

        /* Check all 8 directions */
        for (unsigned char dir = 0; dir < 8; dir++) {
            int newX = (int)player->x + DX[dir];
            int newY = (int)player->y + DY[dir];

            if (newX >= 0 && newX < (int)state->width && 
                newY >= 0 && newY < (int)state->height) {
                int cellValue = BOARD_AT(state->board, state->width, newY, newX);
                if (cellValue > 0) {
                    hasValidMove = 1;
                    break;
                }
            }
        }

        player->valid = hasValidMove ? true : false;
        if (hasValidMove) {
            activeCount++;
        }
    }

    if (activeCount == 0) {
        state->gameOver = true;
    }
}

/* Print final results */
static void printResults(GameState *state) {
    printf("\n=== Game Over ===\n");
    for (unsigned char i = 0; i < state->playersCount; i++) {
        printf("%s: %u points (valid: %u, invalid: %u)\n",
               state->players[i].playerName,
               state->players[i].playerScore,
               state->players[i].validMovements,
               state->players[i].invalidMovements);
    }
}

/* Cleanup resources */
static void cleanup(PlayerProcess *processes, int count, GameState *state, semaphoresStatus *sync, int stateFd, int syncFd, size_t width, size_t height) {
    /* Set game over flag */
    state->gameOver = true;

    /* Close pipes and wait for player processes */
    for (int i = 0; i < count; i++) {
        close(processes[i].pipeFd);
        waitpid(processes[i].pid, NULL, 0);
    }

    /* Destroy and close shared memory */
    (void)syncDestroy(sync, (unsigned int)count);
    (void)syncClose(syncFd, sync);
    (void)syncUnlink();
    (void)stateClose(stateFd, state, width, height);
    (void)stateUnlink();
}
