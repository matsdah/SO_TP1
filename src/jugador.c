#include <playerAI.h>

int main(int argc, char *argv[]){
    if(argc < 3){
        /* Muestro mensaje de error por parametros incorrectos. */
        fprintf(stderr, "Uso minimo de los parámetros: %s <width> <height>\n", argv[0]);
        return 1;
    }

    size_t width = atoi(argv[1]);
    size_t height = atoi(argv[2]);

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

    /* Loop principal del jugador */
    while(1){ 
        acquireReadLock(sync);

        if(state->gameOver){
            releaseReadLock(sync);
            break;
        }

        unsigned char move = findBestMove(state, myIndex);

        releaseReadLock(sync);

        if (write(STDOUT_FILENO, &move, 1) != 1) {
            break;
        }

        sem_wait(&sync->playerSem[myIndex]);

        if (state->gameOver) {
            break;
        }
    }

    syncClose(syncFd, sync);
    stateClose(stateFd, state, width, height);

    return 0;
}