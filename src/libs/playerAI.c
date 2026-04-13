#include <playerAI.h>

/* Direcciones: 
** 0 = Norte
** 1 = Noreste
** 2 = Este
** 3 = Sureste
** 4 = Sur
** 5 = Suroeste
** 6 = Oeste
** 7 = Noroeste 
*/

static const int DY[] = {-1,    -1,    0,     1,    1,      1,      0,  -1};
static const int DX[] = {0,     1,     1,     1,    0,     -1,     -1,  -1};


int findMyIndex(const GameState *state){
    pid_t myPid = getpid();     

    /* Buscar el índice del jugador actual en el estado del juego */
    for(int i = 0; i < state->playerCount; i++){
        if(state->players[i].pid == myPid){
            return i;
        }
    }

    return -1;      /* No encontre el indice actual del jugador.*/
}

unsigned char findBestMove(const GameState *state, int myIndex){

    int myX = state->players[myIndex].x;
    int myY = state->players[myIndex].y;
    int bestDir = -1;
    int bestValue = -1;

    /* Recorre todas las direcciones y calcula la mejor posibilidad para moverse. */
    for(int dir = 0; dir < DIRECTION_COUNT; dir++){
        int nx = myX + DX[dir];
        int ny = myY + DY[dir];

        if((nx < 0) || (nx >= state->width) || (ny < 0) || (ny >= state->height)){
            continue;
        }

        int cell = BOARD_AT(state->board, state->width, ny, nx);    /* Guardo el valor de la celda. */

        if((cell > 0) && (cell > bestValue)){
            /* Actualizo la mejor opción. */
            bestValue = cell;
            bestDir = dir;
        }
    }

    return ((bestDir == -1) ? 0 : (unsigned char)bestDir);     /* Si no hay movimientos válidos, devuelve 0 (Norte). */
}

int checkArguments(char * argv[], size_t * width, size_t *height){
    char *endptr;

    /* Chequeo el ancho del tablero. */
    errno = 0;
    long w = strtol(argv[1], &endptr, 10);
    if((errno == ERANGE) || (*endptr != '\0') || (w <= 0) || (w > USHRT_MAX)){
        fprintf(stderr, "Error: Invalid width '%s'. Must be a positive integer less than or equal to %u.\n", argv[1], USHRT_MAX);
        return -1;
    }

    /* Chequeo el alto del tablero. */
    errno = 0;
    long h = strtol(argv[2], &endptr, 10);
    if((errno == ERANGE) || (*endptr != '\0') || (h <= 0) || (h > USHRT_MAX)){
        fprintf(stderr, "Error: Invalid height '%s'. Must be a positive integer less than or equal to %u.\n", argv[2], USHRT_MAX);
        return -1;
    }

    *width = (size_t)w;
    *height = (size_t)h;
    return 0;   /* Argumentos válidos. */

}

void runLoop(const GameState *state, SyncData *sync, int myIndex){
    int shouldExit = 0;
    while(!shouldExit){
        if(acquireReadLock(sync) == -1){
            break;
        }
        int gameOver = state->gameOver;
        unsigned char move = findBestMove(state, myIndex);
        if(releaseReadLock(sync) == -1){
            break;
        }

        if(gameOver){
            shouldExit = 1;
        } else {
            ssize_t bytesWritten;
            do {
                bytesWritten = write(STDOUT_FILENO, &move, 1);
            } while ((bytesWritten == -1) && (errno == EINTR));

            if (bytesWritten != 1) {
                shouldExit = 1;
            } else {
                int semResult;
                do {
                    semResult = sem_wait(&sync->playerSem[myIndex]);
                } while ((semResult == -1) && (errno == EINTR));

                if(semResult == -1){
                    shouldExit = 1;
                    continue;
                }

                if(acquireReadLock(sync) == -1){
                    break;
                }
                gameOver = state->gameOver;
                if(releaseReadLock(sync) == -1){
                    break;
                }
                if(gameOver){
                    shouldExit = 1;
                }
            }
        }
    }
}
