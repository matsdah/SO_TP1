#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include "shared_data.h"

#define UNUSED(x) (void)(x)

static game_state_t *game_state = NULL;
static sync_data_t *sync_data = NULL;
static int shm_state_fd = -1;
static int shm_sync_fd = -1;
static size_t state_size = 0;

static sem_t *sem_state_changed = NULL;
static sem_t *sem_view_done = NULL;

void cleanup(void);
void signal_handler(int sig);
bool connect_to_shared_memory(void);
void print_board(void);
void print_game_state(void);
void wait_for_state_change(void);
void notify_master_done(void);

void cleanup(void) {
    if (sem_state_changed) sem_close(sem_state_changed);
    if (sem_view_done) sem_close(sem_view_done);
    
    if (sync_data) {
        munmap(sync_data, sizeof(sync_data_t));
    }
    if (game_state) {
        munmap(game_state, state_size);
    }
    if (shm_state_fd != -1) close(shm_state_fd);
    if (shm_sync_fd != -1) close(shm_sync_fd);
}

void signal_handler(int sig) {
    UNUSED(sig);
    cleanup();
    exit(EXIT_FAILURE);
}

bool connect_to_shared_memory(void) {
    shm_state_fd = shm_open(SHM_STATE_NAME, O_RDONLY, 0666);
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

    state_size = sizeof(game_state_t) + game_state->width * game_state->height;
    if (munmap(game_state, sizeof(game_state_t)) == -1) {
        perror("munmap initial");
    }
    
    game_state = mmap(NULL, state_size, PROT_READ, MAP_SHARED, shm_state_fd, 0);
    close(shm_state_fd);
    if (game_state == MAP_FAILED) {
        perror("mmap game_state full");
        return false;
    }

    shm_sync_fd = shm_open(SHM_SYNC_NAME, O_RDWR, 0666);
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

    return true;
}

void print_board(void) {
    int width = game_state->width;
    int height = game_state->height;

    printf("  ");
    for (int x = 0; x < width; x++) {
        printf("%2d", x);
    }
    printf("\n");

    for (int y = 0; y < height; y++) {
        printf("%2d ", y);
        for (int x = 0; x < width; x++) {
            char cell = game_state->board[y * width + x];
            if (cell > 0) {
                printf("%2d ", cell);
            } else if (cell < 0) {
                printf(" P%d", -cell);
            } else {
                printf("  .");
            }
        }
        printf("\n");
    }
}

void print_game_state(void) {
    printf("\n=== Estado del Juego ===\n");
    printf("Tablero: %dx%d | Jugadores: %d | Terminado: %s\n\n",
           game_state->width, game_state->height,
           game_state->player_count,
           game_state->is_finished ? "SI" : "NO");

    print_board();

    printf("\n--- Jugadores ---\n");
    for (int i = 0; i < game_state->player_count; i++) {
        player_t *p = &game_state->players[i];
        printf("%-10s | Score: %3u | Pos: (%2d,%2d) | Validos: %2u | Invalidos: %2u | Bloqueado: %s\n",
               p->name, p->score, p->x, p->y, 
               p->valid_moves, p->invalid_moves,
               p->is_blocked ? "SI" : "NO");
    }
    printf("\n");
}

void wait_for_state_change(void) {
    sem_wait(sem_state_changed);
}

void notify_master_done(void) {
    sem_post(sem_view_done);
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

    while (!game_state->is_finished) {
        wait_for_state_change();
        
        if (game_state->is_finished) {
            print_game_state();
            notify_master_done();
            break;
        }

        print_game_state();
        notify_master_done();
    }

    print_game_state();
    
    cleanup();
    return EXIT_SUCCESS;
}
