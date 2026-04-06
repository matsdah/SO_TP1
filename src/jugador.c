#include <shmState.h>
#include <shmSync.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

/* Direcciones: 0=UP, 1=UP_RIGHT, 2=RIGHT, 3=DOWN_RIGHT, 4=DOWN, 5=DOWN_LEFT, 6=LEFT, 7=UP_LEFT */
static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

int find_my_index(GameState *state){
    pid_t my_pid = getpid();
    for (int i = 0; i < state->playersCount; i++) {
        if (state->players[i].playerPID == my_pid) {
            return i;
        }
    }
    return -1;
}

void acquire_read_lock(semaphoresStatus *sync) {
    sem_wait(&sync->masterMutex);
    sem_wait(&sync->readCountMutex);
    
    sync->playersReadingStatus++;
    if (sync->playersReadingStatus == 1) {
        sem_wait(&sync->gameStateMutex);
    }
    
    sem_post(&sync->readCountMutex);
    sem_post(&sync->masterMutex);
}

void release_read_lock(semaphoresStatus *sync) {
    sem_wait(&sync->readCountMutex);
    
    sync->playersReadingStatus--;
    if (sync->playersReadingStatus == 0) {
        sem_post(&sync->gameStateMutex);
    }
    
    sem_post(&sync->readCountMutex);
}

unsigned char find_best_move(GameState *state, int my_index) {
    int my_x = state->players[my_index].x;
    int my_y = state->players[my_index].y;
    int best_dir = -1;
    int best_value = -1;
    
    for (int dir = 0; dir < 8; dir++) {
        int nx = my_x + dx[dir];
        int ny = my_y + dy[dir];
        
        /* Verificar límites */
        if (nx < 0 || nx >= state->width || ny < 0 || ny >= state->height) {
            continue;
        }
        
        /* Obtener valor de la celda */
        int cell = BOARD_AT(state->board, state->width, ny, nx);
        
        /* Solo celdas libres (valor positivo) */
        if (cell > 0 && cell > best_value) {
            best_value = cell;
            best_dir = dir;
        }
    }
    
    /* Si no hay movimientos válidos, enviar 0 (UP) como movimiento inválido */
    return (best_dir == -1) ? 0 : (unsigned char)best_dir;
}

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