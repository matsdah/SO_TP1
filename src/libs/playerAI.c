#include <playerAI.h>
#include <shmSync.h>
#include <unistd.h>
#include <semaphore.h>

/* Direcciones: 0=UP, 1=UP_RIGHT, 2=RIGHT, 3=DOWN_RIGHT, 4=DOWN, 5=DOWN_LEFT, 6=LEFT, 7=UP_LEFT */
static const int DX[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int DY[] = {-1, -1, 0, 1, 1, 1, 0, -1};

int findMyIndex(GameState *state) {
    pid_t myPid = getpid();
    for (int i = 0; i < state->playerCount; i++) {
        if (state->players[i].pid == myPid) {
            return i;
        }
    }
    return -1;
}

unsigned char findBestMove(GameState *state, int myIndex) {
    int myX = state->players[myIndex].x;
    int myY = state->players[myIndex].y;
    int bestDir = -1;
    int bestValue = -1;

    for (int dir = 0; dir < 8; dir++) {
        int nx = myX + DX[dir];
        int ny = myY + DY[dir];

        if (nx < 0 || nx >= state->width || ny < 0 || ny >= state->height) {
            continue;
        }

        int cell = BOARD_AT(state->board, state->width, ny, nx);

        if (cell > 0 && cell > bestValue) {
            bestValue = cell;
            bestDir = dir;
        }
    }

    return (bestDir == -1) ? 0 : (unsigned char)bestDir;
}
