#include <shmState.h>
#include <shmSync.h>
#include <playerAI.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
        syncClose(syncFd, sync);
        return 1;
    }

    /* Encontrar mi índice */
    int my_index = find_my_index(state);
    if (my_index == -1) {
        fprintf(stderr, "Error: Player not found in game state\n");
        stateClose(stateFd, state, width, height);
        syncClose(syncFd, sync);
        return 1;
    }

    /* Bucle principal del jugador */
    while (1) {
        /* Adquirir lock de lectura */
        acquire_read_lock(sync);
        
        /* Verificar si el juego terminó */
        if (state->gameOver) {
            release_read_lock(sync);
            break;
        }
        
        /* Encontrar el mejor movimiento */
        unsigned char move = find_best_move(state, my_index);
        
        /* Liberar lock de lectura */
        release_read_lock(sync);
        
        /* Enviar movimiento al master por STDOUT */
        if (write(STDOUT_FILENO, &move, 1) != 1) {
            perror("write");
            break;
        }
        
        /* Esperar ACK del master */
        sem_wait(&sync->playersAllowedToMove[my_index]);
    }

    stateClose(stateFd, state, width, height);
    syncClose(syncFd, sync);
    return 0;
}
