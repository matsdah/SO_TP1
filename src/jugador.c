#include <playerAI.h>
#include <shmState.h>
#include <shmSync.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
        return 1;
    }
    
    size_t width, height;
    int checkResult = checkArguments(argv, &width, &height);

    if (checkResult == -1) {
        return 1;
    }

    int syncFd = -1;
    int stateFd = -1;

    SyncData *sync = NULL;
    GameState *state = NULL;

    if (syncOpen(&syncFd, &sync) == -1) {
        perror("Error en syncOpen");
        return 1;
    }

    if (stateOpen(&stateFd, &state, width, height) == -1) {
        perror("Error en stateOpen");
        syncClose(syncFd, sync);
        return 1;
    }

    /* Encontrar mi indice */
    int myIndex = findMyIndex(state);

    if (myIndex == -1) {
        fprintf(stderr, "Error: Jugador no encontrado en el estado del juego.\n");
        stateClose(stateFd, state, width, height);
        syncClose(syncFd, sync);
        return 1;
    }

    runLoop(state, sync, myIndex);

    syncClose(syncFd, sync);
    stateClose(stateFd, state, width, height);

    return 0;
}
