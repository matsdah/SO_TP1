#include "../include/shmState.h"
#include "../include/shmSync.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <height> <width>\n", argv[0]);
        return 1;
    }

    size_t height = (size_t)atoi(argv[1]);
    size_t width = (size_t)atoi(argv[2]);

    int syncFd = -1;
    int stateFd = -1;
    semaphoresStatus *sync = NULL;
    GameState *state = NULL;

    if (syncOpen(&syncFd, &sync) == -1) {
        perror("syncOpen");
        return 1;
    }
    if (stateOpen(&stateFd, &state, width, height) == -1) {
        perror("stateOpen");
        return 1;
    }

    /* Lógica del jugador pendiente; por ahora solo valida IPC y termina. */
    (void)sync;
    (void)state;

    (void)syncClose(syncFd, sync);
    (void)stateClose(stateFd, state, width, height);
    return 0;
}
