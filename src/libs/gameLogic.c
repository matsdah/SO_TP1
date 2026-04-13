#include <gameLogic.h>
#include <shmState.h>
#include <shmSync.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>

/* 
** gameLogic.c -> implementa la lógica del juego, incluyendo la inicialización aleatoria del tablero, 
** spawn de jugadores, validación y aplicacion de movimientos y deteccion de fin de juego. 
*/

/* Vectores de direccion (0=UP, 1=UP_RIGHT, ..., 7=UP_LEFT) */
static const int DX[DIRECTION_COUNT] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[DIRECTION_COUNT] = {-1, -1, 0, 1, 1, 1, 0, -1};

static int semWaitWithInterrupt(sem_t *sem, volatile sig_atomic_t *interrupted){
    while(sem_wait(sem) == -1){
        if(errno == EINTR){
            if(*interrupted){
                return 1;
            }
            continue;
        }

        return -1;
    }

    return 0;
}

static int lockStateForWrite(SyncData *sync, volatile sig_atomic_t *interrupted){
    int lockRet = semWaitWithInterrupt(&sync->masterMutex, interrupted);
    if(lockRet == 1){
        return 1;
    }
    if(lockRet == -1){
        perror("Error en sem_wait masterMutex");
        return -1;
    }

    lockRet = semWaitWithInterrupt(&sync->stateMutex, interrupted);
    if(lockRet == 1){
        if(sem_post(&sync->masterMutex) == -1){
            perror("Error en sem_post masterMutex");
        }
        return 1;
    }
    if(lockRet == -1){
        perror("Error en sem_wait stateMutex");
        if(sem_post(&sync->masterMutex) == -1){
            perror("Error en sem_post masterMutex");
        }
        return -1;
    }

    return 0;
}

static int unlockStateForWrite(SyncData *sync){
    int hasError = 0;

    if(sem_post(&sync->stateMutex) == -1){
        perror("Error en sem_post stateMutex");
        hasError = 1;
    }
    if(sem_post(&sync->masterMutex) == -1){
        perror("Error en sem_post masterMutex");
        hasError = 1;
    }

    return (hasError ? -1 : 0);
}

static int withWriteLockSetGameOver(SyncData *sync, volatile sig_atomic_t *interrupted, GameState *state){
    int lockRet = lockStateForWrite(sync, interrupted);
    if(lockRet != 0){
        return -1;
    }

    state->gameOver = true;

    if(unlockStateForWrite(sync) == -1){
        return -1;
    }

    return 0;
}

static void setTimeoutFromDelayMs(struct timeval *tv, size_t delayMs){
    tv->tv_sec = (time_t)(delayMs / 1000);
    tv->tv_usec = (suseconds_t)((delayMs % 1000) * 1000);
}

static int sleepMsWithInterrupt(size_t delayMs, volatile sig_atomic_t *interrupted){
    struct timespec req;
    struct timespec rem;

    req.tv_sec = (time_t)(delayMs / 1000);
    req.tv_nsec = (long)((delayMs % 1000) * 1000000L);

    while(nanosleep(&req, &rem) == -1){
        if(errno == EINTR){
            if(*interrupted){
                return 1;
            }
            req = rem;
            continue;
        }
        return -1;
    }

    return 0;
}

static size_t elapsedMs(const struct timespec *start, const struct timespec *end){
    time_t sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    if(nsec < 0){
        sec--;
        nsec += 1000000000L;
    }

    return (size_t)sec * 1000U + (size_t)(nsec / 1000000L);
}

void initializeBoard(GameState *state, unsigned int seed){
    /* Semilla para generación de números aleatorios. */
    srand(seed);    

    size_t totalCells = (size_t)state->width * (size_t)state->height;

    for(size_t i = 0; i < totalCells; i++){
        state->board[i] = ((rand() % 9) + 1);   /* Valores aleatorios entre 1 y 9 */
    }
}

void placePlayers(GameState *state){
    unsigned char i;
    unsigned short w = state->width;
    unsigned short h = state->height;
    unsigned char count = state->playerCount;
    size_t totalCells = (size_t)w * (size_t)h;

    for(i = 0; i < count; i++){
        size_t linearIndex = ((size_t)i * totalCells) / (size_t)count;
        state->players[i].x = (unsigned short)(linearIndex % w);
        state->players[i].y = (unsigned short)(linearIndex / w);
        state->players[i].score = 0;
        state->players[i].validMoves = 0;
        state->players[i].invalidMoves = 0;
        state->players[i].isValid = true;

        snprintf(state->players[i].name, NAME_DIM, "Player %d", i + 1);

        /* Marca posicion inicial como ocupada */
        int *cell = &BOARD_AT(state->board, w, state->players[i].y, state->players[i].x);
        state->players[i].score += *cell;
        *cell = -(int)(i + 1);
    }
}

int spawnPlayers(Params *params, PlayerProcess *processes, GameState *state){
    for(size_t i = 0; i < params->playerCount; i++){
        int pipeFds[2];

        if(pipe(pipeFds) == -1){
            /* Fallo la creacion del pipe, muestro mensaje de error. */
            perror("Fallo en la creacion del pipe.");
            return -1;
        }

        pid_t pid = fork();

        if(pid == -1){
            /* Fallo la creacion del proceso hijo, muestro mensaje de error. */
            perror("Fallo en la creacion del proceso hijo.");
            close(pipeFds[0]);
            close(pipeFds[1]);
            return -1;
        }

        if(pid == 0){
            /* Proceso hijo */
            close(pipeFds[0]);

            if(dup2(pipeFds[1], STDOUT_FILENO) == -1){
                perror("Fallo en la duplicacion del descriptor de archivo.");
                close(pipeFds[1]);
                _exit(1);
            }

            close(pipeFds[1]);

            char widthStr[U16_STR_LEN];
            char heightStr[U16_STR_LEN];

            snprintf(widthStr, sizeof(widthStr), "%u", state->width);
            snprintf(heightStr, sizeof(heightStr), "%u", state->height);

            execl(params->players[i], params->players[i], widthStr, heightStr, NULL);
            perror("Fallo en la ejecucion del jugador.");
            _exit(1);

        }else{
            /* Proceso padre */
            close(pipeFds[1]);

            processes[i].pid = pid;
            processes[i].pipeFd = pipeFds[0];

            state->players[i].pid = pid;
        }
    }

    return 0;
}

int setupGameResources(const Params *params, int *stateFd, GameState **state, int *syncFd, SyncData **sync){
    if(stateCreate(stateFd, state, params->width, params->height) == -1){
        perror("Error en stateCreate.");
        return -1;
    }

    if(syncCreate(syncFd, sync) == -1){
        perror("Error en syncCreate.");
        stateClose(*stateFd, *state, params->width, params->height);
        stateUnlink();
        return -1;
    }

    if(syncInit(*sync, params->playerCount) == -1){
        perror("Error en syncInit.");
        syncClose(*syncFd, *sync);
        syncUnlink();
        stateClose(*stateFd, *state, params->width, params->height);
        stateUnlink();
        return -1;
    }

    return 0;
}

void setupInitialGameState(GameState *state, const Params *params){
    state->width = params->width;
    state->height = params->height;
    state->playerCount = params->playerCount;
    state->gameOver = false;

    initializeBoard(state, params->seed);
    placePlayers(state);
}

void initPlayerProcesses(PlayerProcess *processes, int count){
    for(int i = 0; i < count; i++){
        processes[i].pid = -1;
        processes[i].pipeFd = -1;
    }
}

int spawnViewProcess(const Params *params, pid_t *viewPid, int *viewReady){
    *viewPid = -1;
    *viewReady = 0;

    if(params->view == NULL){
        return 0;
    }

    *viewPid = fork();
    if(*viewPid == 0){
        char widthStr[U16_STR_LEN];
        char heightStr[U16_STR_LEN];

        snprintf(widthStr, sizeof(widthStr), "%hu", params->width);
        snprintf(heightStr, sizeof(heightStr), "%hu", params->height);

        execl(params->view, params->view, widthStr, heightStr, NULL);
        perror("execl view");
        _exit(1);
    }

    if(*viewPid < 0){
        perror("Error en la vista de fork.");
        return -1;
    }

    *viewReady = 1;
    return 0;
}

void runMasterLoop(GameState *gameState, SyncData *gameSync, PlayerProcess *playerProcesses, const Params *params, int *viewReady, volatile sig_atomic_t *interrupted){
    time_t startTime = time(NULL);
    int currentPlayer = 0;
    fd_set readFds;
    struct timeval tv;

    while(!gameState->gameOver && !(*interrupted)){
        struct timespec turnStart;
        struct timespec turnEnd;
        int hasTurnStart = (clock_gettime(CLOCK_MONOTONIC, &turnStart) == 0);

        int lockRet;

        int ret;
        do{
            FD_ZERO(&readFds);
            FD_SET(playerProcesses[currentPlayer].pipeFd, &readFds);
            setTimeoutFromDelayMs(&tv, params->delay);
            ret = select(playerProcesses[currentPlayer].pipeFd + 1, &readFds, NULL, NULL, &tv);
        }while((ret == -1) && (errno == EINTR) && !(*interrupted));

        if(ret == -1){
            perror("Error en select");
            (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
            break;
        }

        if((ret > 0) && FD_ISSET(playerProcesses[currentPlayer].pipeFd, &readFds)){
            unsigned char direction;
            ssize_t bytesRead;

            do{
                bytesRead = read(playerProcesses[currentPlayer].pipeFd, &direction, 1);
            }while((bytesRead == -1) && (errno == EINTR) && !(*interrupted));

            if(bytesRead == -1){
                perror("Error leyendo movimiento del jugador");
                (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                break;
            }

            if(bytesRead == 0){
                errno = EPIPE;
                perror("Pipe de jugador cerrado");
                (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                break;
            }

            if(bytesRead == 1){
                lockRet = semWaitWithInterrupt(&gameSync->masterMutex, interrupted);
                if(lockRet == 1){
                    (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                    break;
                }
                if(lockRet == -1){
                    perror("Error en sem_wait masterMutex");
                    (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                    break;
                }

                lockRet = semWaitWithInterrupt(&gameSync->stateMutex, interrupted);
                if(lockRet == 1){
                    if(sem_post(&gameSync->masterMutex) == -1){
                        perror("Error en sem_post masterMutex");
                    }
                    (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                    break;
                }
                if(lockRet == -1){
                    perror("Error en sem_wait stateMutex");
                    if(sem_post(&gameSync->masterMutex) == -1){
                        perror("Error en sem_post masterMutex");
                    }
                    (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                    break;
                }

                if(validateMove(gameState, currentPlayer, direction)){
                    applyMove(gameState, currentPlayer, direction);
                    gameState->players[currentPlayer].validMoves++;
                    startTime = time(NULL);
                }else{
                    gameState->players[currentPlayer].invalidMoves++;
                }

                if(sem_post(&gameSync->stateMutex) == -1){
                    perror("Error en sem_post stateMutex");
                    (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                    break;
                }
                if(sem_post(&gameSync->masterMutex) == -1){
                    perror("Error en sem_post masterMutex");
                    (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                    break;
                }

                if(*viewReady){
                    if(notifyView(gameSync) == -1){
                        *viewReady = 0;
                    }
                }

                if(sem_post(&gameSync->playerSem[currentPlayer]) == -1){
                    perror("Error en sem_post playerSem");
                    (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                    break;
                }
            }
        }

        /*
        ** Evaluar timeout/fin de juego al final de la iteracion,
        ** luego de intentar procesar el movimiento del jugador actual.
        */
        lockRet = lockStateForWrite(gameSync, interrupted);
        if(lockRet == 1){
            (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
            break;
        }
        if(lockRet == -1){
            (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
            break;
        }

        if(*interrupted){
            gameState->gameOver = true;
        }else{
            checkGameOver(gameState, startTime, params->timeout);
        }

        int gameOverNow = gameState->gameOver;
        if(unlockStateForWrite(gameSync) == -1){
            (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
            break;
        }

        if(gameOverNow){
            break;
        }

        currentPlayer = (currentPlayer + 1) % params->playerCount;

        if(hasTurnStart){
            if(clock_gettime(CLOCK_MONOTONIC, &turnEnd) == 0){
                size_t usedMs = elapsedMs(&turnStart, &turnEnd);
                if(usedMs < params->delay){
                    int sleepRet = sleepMsWithInterrupt(params->delay - usedMs, interrupted);
                    if(sleepRet == -1){
                        perror("Error en nanosleep");
                        (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                        break;
                    }
                    if(sleepRet == 1){
                        (void)withWriteLockSetGameOver(gameSync, interrupted, gameState);
                        break;
                    }
                }
            }
        }
    }
}

int waitViewProcess(pid_t viewPid){
    if(viewPid <= 0){
        return 0;
    }

    int viewStatus;
    pid_t waited;

    do{
        waited = waitpid(viewPid, &viewStatus, 0);
    }while((waited == -1) && (errno == EINTR));

    if(waited == -1){
        perror("Error esperando a la vista");
        return -1;
    }

    if(WIFEXITED(viewStatus)){
        printf("Vista (PID: %d): salió con estado: %d\n", viewPid, WEXITSTATUS(viewStatus));
    }else if(WIFSIGNALED(viewStatus)){
        printf("Vista (PID: %d): abortado por señal: %d\n", viewPid, WTERMSIG(viewStatus));
    }

    return 0;
}

int validateMove(GameState *state, int playerIdx, unsigned char direction){
    if(direction >= DIRECTION_COUNT){
        /* Dirección inválida. */
        return 0;
    }

    Player *player = &state->players[playerIdx];
    const int width = (int)state->width;
    const int height = (int)state->height;

    int newX = player->x + DX[direction];
    int newY = player->y + DY[direction];

    if((newX < 0) || (newX >= width) || (newY < 0) || (newY >= height)){
        /* Fuera de los límites del tablero. */
        return 0;
    }

    int cellValue = BOARD_AT(state->board, state->width, newY, newX);

    if(cellValue < 0){
        /* Celda ya ocupada por otro jugador. */
        return 0;
    }

    return 1;
}

void applyMove(GameState *state, int playerIdx, unsigned char direction){
    Player *player = &state->players[playerIdx];
    int newX = player->x + DX[direction];
    int newY = player->y + DY[direction];

    int *cell = &BOARD_AT(state->board, state->width, newY, newX);

    player->score += *cell;     /* Suma el valor de la celda al puntaje del jugador. */

    *cell = -(playerIdx + 1);   /* Marca la celda como ocupada por el jugador. */

    /* Actualiza la posición del jugador en el tablero. */
    player->x = newX;           
    player->y = newY;
}

int notifyView(SyncData *sync){
    struct timespec deadline;
    if(clock_gettime(CLOCK_REALTIME, &deadline) == -1){
        return -1;
    }
    deadline.tv_sec += 1;

    if(sem_post(&sync->printNeeded) == -1){
        return -1;
    }
    while(sem_timedwait(&sync->renderDone, &deadline) == -1){
        if(errno == EINTR){
            continue;
        }
        if(errno == ETIMEDOUT){
            return -1;
        }
        return -1;
    }

    return 0;
}

void checkGameOver(GameState *state, time_t startTime, size_t timeout){
    if((timeout > 0) && ((time(NULL) - startTime) >= (time_t)timeout)){
        /* Tiempo de juego agotado. */
        state->gameOver = true;
        return;
    }

    int activeCount = 0;
    const int width = (int)state->width;
    const int height = (int)state->height;

    /* Verifica si hay movimientos válidos para cada jugador. */
    for(unsigned char i = 0; i < state->playerCount; i++){
        Player *player = &state->players[i];
        bool hasValidMove = false;

        for(unsigned char dir = 0; dir < DIRECTION_COUNT; dir++){
            int newX = (int)player->x + DX[dir];
            int newY = (int)player->y + DY[dir];

            if((newX >= 0) && (newX < width) && (newY >= 0) && (newY < height)){
                int cellValue = BOARD_AT(state->board, state->width, newY, newX);

                if(cellValue > 0){
                    /* Hay un movimiento válido. */
                    hasValidMove = true;
                    break;     /* Salgo del ultimo for. */
                }
            }
        }

        player->isValid = hasValidMove;

        if(hasValidMove){
            activeCount++;
        }
    }

    if(activeCount == 0){
        /* No hay movimientos válidos para ninguno de los jugadores. */
        state->gameOver = true;
    }
}

void printResults(GameState *state){
    printf("\n⚪⚪⚪⚪⚪ ¡Juego Terminado! ⚪⚪⚪⚪⚪\n");

    for(unsigned char i = 0; i < state->playerCount; i++){
        printf("%s: %u Puntos (Válidos: %u, Inválidos: %u)\n",
               state->players[i].name,
               state->players[i].score,
               state->players[i].validMoves,
               state->players[i].invalidMoves);
    }
}

void cleanup(PlayerProcess *processes, int count, GameState *state, SyncData *sync, int stateFd, int syncFd, size_t width, size_t height, pid_t viewPid, int interrupted){
    if(state != NULL && sync != NULL){
        state->gameOver = true;
        /* Desbloquea a todos los jugadores para que vean gameOver */
        for(int i = 0; i < count; i++){
            if(sem_post(&sync->playerSem[i]) == -1){
                perror("Error en sem_post playerSem");
            }
        }
    }
    /* Notifica a la vista */
    if(sync != NULL && viewPid > 0 && !interrupted){
        (void)notifyView(sync);
    }


    printf("\n⚪⚪⚪⚪⚪ Estado de Salida de Procesos Hijos ⚪⚪⚪⚪⚪\n");

    for(int i = 0; i < count; i++){
        int status;     /* Variable para almacenar el estado de salida del proceso hijo. */

        if(processes[i].pipeFd >= 0){
            close(processes[i].pipeFd);     /* Cierra el descriptor de archivo del pipe. */
        }
        if(processes[i].pid > 0){
            pid_t waited;
            do{
                waited = waitpid(processes[i].pid, &status, 0);      /* Espera a que el proceso hijo termine. */
            }while((waited == -1) && (errno == EINTR));

            if(waited == -1){
                perror("Error esperando jugador");
                continue;
            }
            /* Imprime el estado de salida del proceso hijo. */
            if(WIFEXITED(status)){
                printf("Jugador N°%d (PID: %d): salió con estado %d\n", i + 1, processes[i].pid, WEXITSTATUS(status));
            }else{
                if(WIFSIGNALED(status)){
                    printf("Jugador N°%d (PID: %d): murió por señal %d\n", i + 1, processes[i].pid, WTERMSIG(status));
                }
            }
        }
    }

    if(sync != NULL && syncFd >= 0){
        /* Libera los recursos de sincronización. */
        syncDestroy(sync, count);
        syncClose(syncFd, sync);
        syncUnlink();
    }

    if(state != NULL && stateFd >= 0){
        /* Libera los recursos del estado del juego. */
        stateClose(stateFd, state, width, height);
        stateUnlink();
    }
}
