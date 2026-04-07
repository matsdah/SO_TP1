#include <gameLogic.h>

/* Vectores de direccion (0=UP, 1=UP_RIGHT, ..., 7=UP_LEFT) */
static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

void initializeBoard(GameState *state, unsigned int seed) {
    srand(seed);

    size_t totalCells = (size_t)state->width * state->height;

    for (size_t i = 0; i < totalCells; i++) {
        state->board[i] = (rand() % 9) + 1;
    }
}

void placePlayers(GameState *state) {
    unsigned char i;
    unsigned short w = state->width;
    unsigned short h = state->height;
    unsigned char count = state->playerCount;

    for (i = 0; i < count; i++) {
        state->players[i].x = ((i * w) / count);
        state->players[i].y = ((i * h) / count);
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

int spawnPlayers(Params *params, PlayerProcess *processes, GameState *state) {
    for (size_t i = 0; i < params->playerCount; i++) {
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
            /* Proceso hijo */
            close(pipeFds[0]);

            if (dup2(pipeFds[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }

            close(pipeFds[1]);

            char widthStr[16];
            char heightStr[16];

            snprintf(widthStr, sizeof(widthStr), "%u", state->width);
            snprintf(heightStr, sizeof(heightStr), "%u", state->height);

            execl(params->players[i], params->players[i], widthStr, heightStr, NULL);
            perror("execl player");
            exit(1);

        } else {
            /* Proceso padre */
            close(pipeFds[1]);

            processes[i].pid = pid;
            processes[i].pipeFd = pipeFds[0];

            state->players[i].pid = pid;
        }
    }

    return 0;
}

int validateMove(GameState *state, int playerIdx, unsigned char direction) {
    if (direction >= 8) {
        return 0;
    }

    Player *player = &state->players[playerIdx];

    int newX = player->x + DX[direction];
    int newY = player->y + DY[direction];

    if (newX < 0 || (newX >= state->width) || newY < 0 || (newY >= state->height)) {
        return 0;
    }

    int cellValue = BOARD_AT(state->board, state->width, newY, newX);

    if (cellValue < 0) {
        return 0;
    }

    return 1;
}

void applyMove(GameState *state, int playerIdx, unsigned char direction) {
    Player *player = &state->players[playerIdx];
    int newX = player->x + DX[direction];
    int newY = player->y + DY[direction];

    int *cell = &BOARD_AT(state->board, state->width, newY, newX);

    player->score += *cell;

    *cell = -(playerIdx + 1);

    player->x = newX;
    player->y = newY;
}

void notifyView(SyncData *sync) {
    sem_post(&sync->printNeeded);
    sem_wait(&sync->renderDone);
}

void checkGameOver(GameState *state, time_t startTime, size_t timeout) {
    if (timeout > 0 && (time(NULL) - startTime) >= (time_t)timeout) {
        state->gameOver = true;
        return;
    }

    int activeCount = 0;

    for (unsigned char i = 0; i < state->playerCount; i++) {
        Player *player = &state->players[i];
        int hasValidMove = 0;

        for (unsigned char dir = 0; dir < 8; dir++) {
            int newX = (int)player->x + DX[dir];
            int newY = (int)player->y + DY[dir];

            if ((newX >= 0) && (newX < (int)state->width) && 
                (newY >= 0) && (newY < (int)state->height)) {
                int cellValue = BOARD_AT(state->board, state->width, newY, newX);

                if (cellValue > 0) {
                    hasValidMove = 1;
                    break;
                }
            }
        }

        player->isValid = hasValidMove ? true : false;

        if (hasValidMove) {
            activeCount++;
        }
    }

    if (activeCount == 0) {
        state->gameOver = true;
    }
}

void printResults(GameState *state) {
    printf("\n=== Game Over! ===\n");

    for (unsigned char i = 0; i < state->playerCount; i++) {
        printf("%s: %u points (valid: %u, invalid: %u)\n",
               state->players[i].name,
               state->players[i].score,
               state->players[i].validMoves,
               state->players[i].invalidMoves);
    }
}

void cleanup(PlayerProcess *processes, int count, GameState *state, SyncData *sync, 
             int stateFd, int syncFd, size_t width, size_t height, pid_t viewPid) {
    state->gameOver = true;

    /* Desbloquea a todos los jugadores para que vean gameOver */
    for (int i = 0; i < count; i++) {
        sem_post(&sync->playerSem[i]);
    }

    /* Notifica a la vista */
    if (viewPid > 0) {
        notifyView(sync);
    }

    printf("\n=== Child Process Exit Status ===\n");

    for (int i = 0; i < count; i++) {
        int status;

        close(processes[i].pipeFd);

        waitpid(processes[i].pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("Player %d (PID %d): exited with status %d\n",
                   i + 1, processes[i].pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Player %d (PID %d): killed by signal %d\n",
                   i + 1, processes[i].pid, WTERMSIG(status));
        }
    }

    syncDestroy(sync, count);
    syncClose(syncFd, sync);
    syncUnlink();
    stateClose(stateFd, state, width, height);
    stateUnlink();
}
