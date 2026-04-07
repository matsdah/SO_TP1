#include <gameLogic.h>

/* Direction vectors (0=UP, 1=UP_RIGHT, ..., 7=UP_LEFT) */
static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

void initializeBoard(GameState *state, unsigned int seed){
    /* Setea una semilla aleatoria en cada iteracion. */
    srand(seed);
    
    size_t totalCells = (size_t)state->width * state->height;

    for(size_t i = 0; i < totalCells; i++){
        /* Rellena el tablero. */
        state->board[i] = (rand() % 9) + 1;
    }
}

void placePlayers(GameState *state){

    unsigned char i;
    unsigned short w = state->width;
    unsigned short h = state->height;
    unsigned char count = state->playersCount;

    /* Dsitribuye los jugadores de manera uniforme en el tablero. */
    for(i = 0; i < count; i++){

        state->players[i].x = ((i * w) / count);
        state->players[i].y = ((i * h) / count);
        state->players[i].playerScore = 0;
        state->players[i].validMovements = 0;
        state->players[i].invalidMovements = 0;
        state->players[i].valid = true;

        snprintf(state->players[i].playerName, NAME_DIM, "Player %d", i + 1);
        
        /* Posicion inicial como ocupada. */
        int *cell = &BOARD_AT(state->board, w, state->players[i].y, state->players[i].x);
        state->players[i].playerScore += *cell;
        *cell = -(int)(i + 1);
    }
}

int spawnPlayers(Parameters *params, PlayerProcess *processes, GameState *state){

    for(size_t i = 0; i < params->amount_players; i++){
        int pipeFds[2];

        if(pipe(pipeFds) == -1){
            perror("Error en Pipes.");
            return -1;
        }

        pid_t pid = fork();
        if(pid == -1){
            perror("Error en Fork.");
            return -1;
        }

        if(pid == 0){
            /* Proceso hijo. */
            close(pipeFds[0]); /* Close read end */
            
            /* Redirect stdout to pipe write end */
            if(dup2(pipeFds[1], STDOUT_FILENO) == -1){
                perror("Error en dup2.");
                exit(1);
            }

            close(pipeFds[1]);

            /* Execute player */
            char widthStr[16];
            char heightStr[16];

            snprintf(widthStr, sizeof(widthStr), "%u", state->width);       /* Convierte el ancho a string. */
            snprintf(heightStr, sizeof(heightStr), "%u", state->height);    /* Convierte la altura a string. */

            execl(params->players[i], params->players[i], widthStr, heightStr, NULL);
            perror("execl player");
            exit(1);

        }else{
            /* Proceso padre. */
            close(pipeFds[1]); /* Close write end */

            processes[i].pid = pid;
            processes[i].pipeFd = pipeFds[0];

            state->players[i].playerPID = pid;
        }
    }

    return 0;
}

int validateMove(GameState *state, int playerIdx, unsigned char direction){

    if(direction >= 8){
        /* Direccion invalida. */
        return 0;
    }

    Player *player = &state->players[playerIdx];

    int newX = player->x + DX[direction];
    int newY = player->y + DY[direction];

    /* Check barreras. */
    if(newX < 0 || (newX >= state->width) || newY < 0 || (newY >= state->height)){
        /* Posicion fuera de los limites. */
        return 0;
    }

    /* Chequea si las celdas ya estan ocupadas. */
    int cellValue = BOARD_AT(state->board, state->width, newY, newX);

    if(cellValue < 0){
        /* Celda ocupada. */
        return 0;
    }

    return 1;
}

void applyMove(GameState *state, int playerIdx, unsigned char direction) {
    Player *player = &state->players[playerIdx];
    int newX = player->x + DX[direction];
    int newY = player->y + DY[direction];

    /* Obtiene el valor de la celda. */
    int *cell = &BOARD_AT(state->board, state->width, newY, newX);

    /* Agregar el valor obtenido al score. */
    player->playerScore += *cell;

    /* Marca la celda como ocupada. */
    *cell = -(playerIdx + 1);

    /* Actualiza la posición del jugador. */
    player->x = newX;
    player->y = newY;
}

void notifyView(semaphoresStatus *sync){
    /* Incrementa el semaforo printNeeded. */
    sem_post(&sync->printNeeded);

    /* Decrementa el semaforo renderDone. */
    sem_wait(&sync->renderDone);
}

void checkGameOver(GameState *state, time_t startTime, size_t timeout){

    /* Check timeout. */
    if(timeout > 0 && (time(NULL) - startTime) >= (time_t)timeout){
        state->gameOver = true;
        return;
    }

    /* Chequea si todos los jugadores estan bloqueados. */
    int activeCount = 0;

    for(unsigned char i = 0; i < state->playersCount; i++){

        Player *player = &state->players[i];
        int hasValidMove = 0;

        /* Verifica si el jugador tiene movimientos válidos. */
        for(unsigned char dir = 0; dir < 8; dir++) {
            int newX = (int)player->x + DX[dir];
            int newY = (int)player->y + DY[dir];

            if((newX >= 0) && (newX < (int)state->width) && (newY >= 0) && (newY < (int)state->height)){
                int cellValue = BOARD_AT(state->board, state->width, newY, newX);

                if(cellValue > 0){
                    /* Hay un movimiento válido. */
                    hasValidMove = 1;
                    break;
                }
            }
        }

        player->valid = hasValidMove ? (true) : (false);

        if(hasValidMove){
            activeCount++;
        }
    }

    if(activeCount == 0){
        /* Todos los jugadores estan bloqueados. */
        state->gameOver = true;
    }
}

void printResults(GameState *state){

    printf("\n=== Game Over! ===\n");

    for(unsigned char i = 0; i < state->playersCount; i++){

        printf("%s: %u points (valid: %u, invalid: %u)\n",
               state->players[i].playerName,
               state->players[i].playerScore,
               state->players[i].validMovements,
               state->players[i].invalidMovements);
    }
}

void cleanup(PlayerProcess *processes, int count, GameState *state, semaphoresStatus *sync, int stateFd, int syncFd, size_t width, size_t height){
    /* Setea el flag de gameover. */
    state->gameOver = true;

    for(int i = 0; i < count; i++){

        /* Cierra pipes. */
        close(processes[i].pipeFd);

        /* Espera a los procesos de los jugadores. */
        waitpid(processes[i].pid, NULL, 0);
    }

    /* Cierra la memoria compartida. */
    syncDestroy(sync, count);
    syncClose(syncFd, sync);
    syncUnlink();
    stateClose(stateFd, state, width, height);
    stateUnlink();
}
