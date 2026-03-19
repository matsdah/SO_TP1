#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <math.h>
#include "shared_data.h"

#define UNUSED(x) (void)(x)

static game_state_t *game_state = NULL;
static sync_data_t *sync_data = NULL;
static int my_index = -1;

static sem_t *sem_state_changed = NULL;
static sem_t *sem_view_done = NULL;
static sem_t *sem_write_queue = NULL;
static sem_t *sem_state_mutex = NULL;
static sem_t *sem_read_count_mutex = NULL;
static sem_t *sem_player_ack = NULL;

void cleanup(void);
void signal_handler(int sig);
bool connect_to_shared_memory(void);
int find_my_index(void);
bool is_valid_move(int x, int y, int dir, int width, int height);
unsigned char find_best_move(void);
void request_read_lock(void);
void release_read_lock(void);
void send_move(unsigned char move);
void wait_for_ack(void);

void cleanup(void) {
    if (sem_state_changed) sem_close(sem_state_changed);
    if (sem_view_done) sem_close(sem_view_done);
    if (sem_write_queue) sem_close(sem_write_queue);
    if (sem_state_mutex) sem_close(sem_state_mutex);
    if (sem_read_count_mutex) sem_close(sem_read_count_mutex);
    if (sem_player_ack) sem_close(sem_player_ack);
    
    if (sync_data) {
        munmap(sync_data, sizeof(sync_data_t));
    }
    if (game_state) {
        munmap(game_state, sizeof(game_state_t) + 
               game_state->width * game_state->height * sizeof(char));
    }
}

void signal_handler(int sig) {
    UNUSED(sig);
    cleanup();
    exit(EXIT_FAILURE);
}

bool connect_to_shared_memory(void) {
    int shm_state_fd = shm_open(SHM_STATE_NAME, O_RDONLY, 0666);
    if (shm_state_fd == -1) {
        perror("shm_open game_state");
        return false;
    }

    game_state = mmap(NULL, sizeof(game_state_t), PROT_READ, MAP_SHARED, shm_state_fd, 0);
    if (game_state == MAP_FAILED) {
        perror("mmap game_state");
        close(shm_state_fd);
        return false;
    }

    size_t state_size = sizeof(game_state_t) + game_state->width * game_state->height;
    if (munmap(game_state, sizeof(game_state_t)) == -1) {
        perror("munmap initial");
    }
    
    game_state = mmap(NULL, state_size, PROT_READ, MAP_SHARED, shm_state_fd, 0);
    close(shm_state_fd);
    if (game_state == MAP_FAILED) {
        perror("mmap game_state full");
        return false;
    }

    int shm_sync_fd = shm_open(SHM_SYNC_NAME, O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open game_sync");
        return false;
    }

    sync_data = mmap(NULL, sizeof(sync_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    close(shm_sync_fd);
    if (sync_data == MAP_FAILED) {
        perror("mmap game_sync");
        return false;
    }

    sem_state_changed = sem_open(SEM_STATE_CHANGED, 0);
    if (sem_state_changed == SEM_FAILED) { perror("sem_open state_changed"); return false; }
    
    sem_view_done = sem_open(SEM_VIEW_DONE, 0);
    if (sem_view_done == SEM_FAILED) { perror("sem_open view_done"); return false; }

    sem_write_queue = sem_open(SEM_WRITE_QUEUE, 0);
    if (sem_write_queue == SEM_FAILED) { perror("sem_open write_queue"); return false; }

    sem_state_mutex = sem_open(SEM_STATE_MUTEX, 0);
    if (sem_state_mutex == SEM_FAILED) { perror("sem_open state_mutex"); return false; }

    sem_read_count_mutex = sem_open(SEM_READ_COUNT_MUTEX, 0);
    if (sem_read_count_mutex == SEM_FAILED) { perror("sem_open read_count_mutex"); return false; }

    return true;
}

int find_my_index(void) {
    pid_t my_pid = getpid();
    for (int i = 0; i < game_state->player_count; i++) {
        if (game_state->players[i].pid == my_pid) {
            return i;
        }
    }
    return -1;
}

bool is_valid_move(int x, int y, int dir, int width, int height) {
    int nx = x, ny = y;
    
    switch (dir) {
        case DIR_UP: ny--; break;
        case DIR_UP_RIGHT: nx++; ny--; break;
        case DIR_RIGHT: nx++; break;
        case DIR_DOWN_RIGHT: nx++; ny++; break;
        case DIR_DOWN: ny++; break;
        case DIR_DOWN_LEFT: nx--; ny++; break;
        case DIR_LEFT: nx--; break;
        case DIR_UP_LEFT: nx--; ny--; break;
        default: return false;
    }

    if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
        return false;
    }

    char cell = game_state->board[ny * width + nx];
    return cell > 0;
}

unsigned char find_best_move(void) {
    int x = game_state->players[my_index].x;
    int y = game_state->players[my_index].y;
    int width = game_state->width;
    int height = game_state->height;

    int best_dir = -1;
    int best_reward = -1;
    double best_distance = 999999.0;

    for (int dir = 0; dir < 8; dir++) {
        if (!is_valid_move(x, y, dir, width, height)) {
            continue;
        }

        int nx = x, ny = y;
        switch (dir) {
            case DIR_UP: ny--; break;
            case DIR_UP_RIGHT: nx++; ny--; break;
            case DIR_RIGHT: nx++; break;
            case DIR_DOWN_RIGHT: nx++; ny++; break;
            case DIR_DOWN: ny++; break;
            case DIR_DOWN_LEFT: nx--; ny++; break;
            case DIR_LEFT: nx--; break;
            case DIR_UP_LEFT: nx--; ny--; break;
        }

        char cell = game_state->board[ny * width + nx];
        int reward = (int)cell;

        if (reward > best_reward) {
            best_reward = reward;
            best_dir = dir;
            best_distance = 0;
        } else if (reward == best_reward && best_dir != -1) {
            double dist = sqrt((double)(nx * nx + ny * ny));
            if (dist < best_distance) {
                best_distance = dist;
                best_dir = dir;
            }
        }
    }

    if (best_dir == -1) {
        return (unsigned char)(rand() % 8);
    }

    return (unsigned char)best_dir;
}

void request_read_lock(void) {
    sem_wait(sem_write_queue);
    sem_wait(sem_read_count_mutex);
    
    sync_data->readers_count++;
    if (sync_data->readers_count == 1) {
        sem_wait(sem_state_mutex);
    }
    
    sem_post(sem_read_count_mutex);
    sem_post(sem_write_queue);
}

void release_read_lock(void) {
    sem_wait(sem_read_count_mutex);
    
    sync_data->readers_count--;
    if (sync_data->readers_count == 0) {
        sem_post(sem_state_mutex);
    }
    
    sem_post(sem_read_count_mutex);
}

void send_move(unsigned char move) {
    write(STDOUT_FILENO, &move, 1);
}

void wait_for_ack(void) {
    sem_wait(sem_player_ack);
}

int main(int argc, char *argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (!connect_to_shared_memory()) {
        fprintf(stderr, "Error: No se pudo conectar a la memoria compartida\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    my_index = find_my_index();
    if (my_index < 0) {
        fprintf(stderr, "Error: No se encontro mi indice de jugador\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    char sem_name[64];
    snprintf(sem_name, sizeof(sem_name), "%s%d", SEM_PLAYER_ACK_PREFIX, my_index);
    sem_player_ack = sem_open(sem_name, 0);
    if (sem_player_ack == SEM_FAILED) {
        perror("sem_open player_ack");
        cleanup();
        exit(EXIT_FAILURE);
    }

    while (!game_state->is_finished) {
        request_read_lock();
        
        if (game_state->is_finished) {
            release_read_lock();
            break;
        }

        unsigned char move = find_best_move();
        
        release_read_lock();

        send_move(move);
        
        wait_for_ack();
    }

    cleanup();
    return EXIT_SUCCESS;
}
