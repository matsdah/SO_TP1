#include <gameLogic.h>
#include <shmState.h>
#include <shmSync.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>

/* 
** gameLogic.c -> implementa la lógica del juego, incluyendo la inicialización aleatoria del tablero, 
** spawn de jugadores, validación y aplicacion de movimientos y deteccion de fin de juego. 
*/

/* Vectores de direccion (0=UP, 1=UP_RIGHT, ..., 7=UP_LEFT) */
static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

void initializeBoard(GameState *state, unsigned int seed){
    /* Semilla para generación de números aleatorios. */
    srand(seed);    

    size_t totalCells = (size_t)(state->width * state->height);

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
                exit(1);
            }

            close(pipeFds[1]);

            char widthStr[16];
            char heightStr[16];

            snprintf(widthStr, sizeof(widthStr), "%u", state->width);
            snprintf(heightStr, sizeof(heightStr), "%u", state->height);

            execl(params->players[i], params->players[i], widthStr, heightStr, NULL);
            perror("Fallo en la ejecucion del jugador.");
            exit(1);

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

int validateMove(GameState *state, int playerIdx, unsigned char direction){
    if(direction >= 8){
        /* Dirección inválida. */
        return 0;
    }

    Player *player = &state->players[playerIdx];

    int newX = player->x + DX[direction];
    int newY = player->y + DY[direction];

    if((newX < 0) || (newX >= state->width) || (newY < 0) || (newY >= state->height)){
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

    sem_post(&sync->printNeeded);       /* Libera el semáforo de impresión. */
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

    /* Verifica si hay movimientos válidos para cada jugador. */
    for(unsigned char i = 0; i < state->playerCount; i++){
        Player *player = &state->players[i];
        bool hasValidMove = false;

        for(unsigned char dir = 0; dir < 8; dir++){
            int newX = (int)player->x + DX[dir];
            int newY = (int)player->y + DY[dir];

            if((newX >= 0) && (newX < state->width) && (newY >= 0) && (newY < state->height)){
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
            sem_post(&sync->playerSem[i]);
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
