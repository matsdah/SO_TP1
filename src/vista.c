#include <shmState.h>
#include <shmSync.h>
#include <viewRender.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
        return 1;
    }

    size_t width = atoi(argv[1]);
    size_t height = atoi(argv[2]);

    int syncFd = -1;
    int stateFd = -1;

    SyncData *sync = NULL;
    GameState *state = NULL;

    if (syncOpen(&syncFd, &sync) == -1) {
        perror("syncOpen");
        return 1;
    }

    if (stateOpen(&stateFd, &state, width, height) == -1) {
        perror("stateOpen");
        syncClose(syncFd, sync);
        return 1;
    }

    /* Loop principal de renderizado */
    while (1) {
        sem_wait(&sync->printNeeded);

        acquireReadLock(sync);

        if (state->gameOver) {
            clearScreen();
            printView(state);
            printStats(state);
            releaseReadLock(sync);
            sem_post(&sync->renderDone);
            break;
        }

        clearScreen();
        printView(state);
        printStats(state);

        releaseReadLock(sync);

        sem_post(&sync->renderDone);
    }

    syncClose(syncFd, sync);
    stateClose(stateFd, state, width, height);

    return 0;
}
