#include <playerAI.h>
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
