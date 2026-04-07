#include <shmState.h>
#include <shmSync.h>
#include <viewRender.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

int main(int argc, char *argv[]){
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <width> <height>\n", argv[0]);
        return 1;
    }

    size_t width = (size_t)atoi(argv[1]);
    size_t height = (size_t)atoi(argv[2]);

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

    while (1) {
        /* Esperar a que el master indique que hay cambios */
        sem_wait(&sync->printNeeded);
        
        /* Adquirir lock de lectura para acceder al estado */
        acquireReadLock(sync);
        
        /* Verificar si el juego terminó */
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
        
        /* Liberar lock de lectura */
        releaseReadLock(sync);

        /* Notificar al master que terminamos de imprimir */
        sem_post(&sync->renderDone);
    }

    (void)syncClose(syncFd, sync);
    (void)stateClose(stateFd, state, width, height);
    return 0;
}
