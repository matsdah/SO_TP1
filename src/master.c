#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include "shared_data.h"

#define UNUSED(x) (void)(x)

typedef struct {
    int pipe_fd;
    pid_t pid;
} player_info_t;

static game_state_t *game_state = NULL;
static sync_data_t *sync_data = NULL;
static player_info_t *players = NULL;
static int player_count = 0;
static int view_pid = -1;
static int shm_state_fd = -1;
static int shm_sync_fd = -1;
static size_t state_size = 0;

static sem_t *sem_state_changed = NULL;
static sem_t *sem_view_done = NULL;
static sem_t *sem_write_queue = NULL;
static sem_t *sem_state_mutex = NULL;
static sem_t *sem_read_count_mutex = NULL;
static sem_t *sem_player_ack[MAX_PLAYERS];

static char sem_name[64];

void cleanup(void);
void signal_handler(int sig);
void print_usage(const char *prog);
bool init_shared_memory(int width, int height);
bool init_sync_primitives(void);
void init_board(int seed);
void distribute_players(void);
bool create_player_processes(char *player_paths[]);
bool create_view_process(const char *view_path);
bool has_valid_moves(int player_idx);
bool handle_player_move(int player_idx);
void notify_view_and_wait(void);
bool all_players_blocked(void);
void finish_game(void);
void process_game_loop(int delay_ms, int timeout_s);

void cleanup(void) {
    if (players) {
        for (int i = 0; i < player_count; i++) {
            if (players[i].pipe_fd != -1) {
                close(players[i].pipe_fd);
            }
        }
        free(players);
    }

    if (sem_state_changed) sem_close(sem_state_changed);
    if (sem_view_done) sem_close(sem_view_done);
    if (sem_write_queue) sem_close(sem_write_queue);
    if (sem_state_mutex) sem_close(sem_state_mutex);
    if (sem_read_count_mutex) sem_close(sem_read_count_mutex);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (sem_player_ack[i]) sem_close(sem_player_ack[i]);
    }

    if (sync_data) {
        munmap(sync_data, sizeof(sync_data_t));
    }

    if (game_state) {
        munmap(game_state, state_size);
    }

    shm_unlink(SHM_STATE_NAME);
    shm_unlink(SHM_SYNC_NAME);

    sem_unlink(SEM_STATE_CHANGED);
    sem_unlink(SEM_VIEW_DONE);
    sem_unlink(SEM_WRITE_QUEUE);
    sem_unlink(SEM_STATE_MUTEX);
    sem_unlink(SEM_READ_COUNT_MUTEX);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        snprintf(sem_name, sizeof(sem_name), "%s%d", SEM_PLAYER_ACK_PREFIX, i);
        sem_unlink(sem_name);
    }

    if (shm_state_fd != -1) close(shm_state_fd);
    if (shm_sync_fd != -1) close(shm_sync_fd);
}

void signal_handler(int sig) {
    UNUSED(sig);
    cleanup();
    exit(EXIT_FAILURE);
}

void print_usage(const char *prog) {
    fprintf(stderr, "Uso: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view] -p player1 [player2 ...]\n", prog);
    fprintf(stderr, "  -w width: Ancho del tablero (default: %d, min: %d)\n", DEFAULT_WIDTH, DEFAULT_WIDTH);
    fprintf(stderr, "  -h height: Alto del tablero (default: %d, min: %d)\n", DEFAULT_HEIGHT, DEFAULT_HEIGHT);
    fprintf(stderr, "  -d delay: Delay en ms (default: %d)\n", DEFAULT_DELAY);
    fprintf(stderr, "  -t timeout: Timeout en segundos (default: %d)\n", DEFAULT_TIMEOUT);
    fprintf(stderr, "  -s seed: Semilla (default: time(NULL))\n");
    fprintf(stderr, "  -v view: Ruta del binario de vista\n");
    fprintf(stderr, "  -p player1 player2 ...: Binarios de jugadores (min: %d, max: %d)\n", MIN_PLAYERS, MAX_PLAYERS);
}

bool init_shared_memory(int width, int height) {
    state_size = sizeof(game_state_t) + width * height * sizeof(char);

    shm_state_fd = shm_open(SHM_STATE_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_state_fd == -1) {
        perror("shm_open game_state");
        return false;
    }

    if (ftruncate(shm_state_fd, state_size) == -1) {
        perror("ftruncate game_state");
        return false;
    }

    game_state = mmap(NULL, state_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_state_fd, 0);
    if (game_state == MAP_FAILED) {
        perror("mmap game_state");
        return false;
    }

    game_state->width = width;
    game_state->height = height;
    game_state->player_count = player_count;
    game_state->is_finished = false;

    shm_sync_fd = shm_open(SHM_SYNC_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_sync_fd == -1) {
        perror("shm_open game_sync");
        return false;
    }

    if (ftruncate(shm_sync_fd, sizeof(sync_data_t)) == -1) {
        perror("ftruncate game_sync");
        return false;
    }

    sync_data = mmap(NULL, sizeof(sync_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if (sync_data == MAP_FAILED) {
        perror("mmap game_sync");
        return false;
    }

    sync_data->readers_count = 0;

    return true;
}

bool init_sync_primitives(void) {
    sem_state_changed = sem_open(SEM_STATE_CHANGED, O_CREAT, 0666, 0);
    if (sem_state_changed == SEM_FAILED) { perror("sem_open state_changed"); return false; }
    
    sem_view_done = sem_open(SEM_VIEW_DONE, O_CREAT, 0666, 0);
    if (sem_view_done == SEM_FAILED) { perror("sem_open view_done"); return false; }
    
    sem_write_queue = sem_open(SEM_WRITE_QUEUE, O_CREAT, 0666, 1);
    if (sem_write_queue == SEM_FAILED) { perror("sem_open write_queue"); return false; }
    
    sem_state_mutex = sem_open(SEM_STATE_MUTEX, O_CREAT, 0666, 1);
    if (sem_state_mutex == SEM_FAILED) { perror("sem_open state_mutex"); return false; }
    
    sem_read_count_mutex = sem_open(SEM_READ_COUNT_MUTEX, O_CREAT, 0666, 1);
    if (sem_read_count_mutex == SEM_FAILED) { perror("sem_open read_count_mutex"); return false; }
    
    for (int i = 0; i < player_count; i++) {
        snprintf(sem_name, sizeof(sem_name), "%s%d", SEM_PLAYER_ACK_PREFIX, i);
        sem_player_ack[i] = sem_open(sem_name, O_CREAT, 0666, 0);
        if (sem_player_ack[i] == SEM_FAILED) { 
            perror("sem_open player_ack"); 
            return false; 
        }
    }
    
    for (int i = player_count; i < MAX_PLAYERS; i++) {
        sem_player_ack[i] = NULL;
    }

    return true;
}

void init_board(int seed) {
    srand(seed);

    for (int y = 0; y < game_state->height; y++) {
        for (int x = 0; x < game_state->width; x++) {
            game_state->board[y * game_state->width + x] = (rand() % 9) + 1;
        }
    }

    for (int i = 0; i < player_count; i++) {
        snprintf(game_state->players[i].name, 16, "Player%d", i + 1);
        game_state->players[i].score = 0;
        game_state->players[i].invalid_moves = 0;
        game_state->players[i].valid_moves = 0;
        game_state->players[i].pid = 0;
        game_state->players[i].is_blocked = false;
    }
}

void distribute_players(void) {
    int cells_per_player = (game_state->width * game_state->height) / player_count;
    int extra_cells = (game_state->width * game_state->height) % player_count;

    int current_cell = 0;
    int width = game_state->width;

    for (int i = 0; i < player_count; i++) {
        int player_cells = cells_per_player + (i < extra_cells ? 1 : 0);
        
        int start_cell = current_cell;
        int y = start_cell / width;
        int x = start_cell % width;

        game_state->players[i].x = x;
        game_state->players[i].y = y;

        game_state->board[y * width + x] = -(i + 1);

        current_cell += player_cells;
    }
}

bool create_player_processes(char *player_paths[]) {
    players = malloc(sizeof(player_info_t) * player_count);
    if (!players) return false;

    for (int i = 0; i < player_count; i++) {
        players[i].pipe_fd = -1;
        players[i].pid = -1;
    }

    for (int i = 0; i < player_count; i++) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return false;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return false;
        }

        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], 1);
            if (pipefd[1] != 1) close(pipefd[1]);

            char width_str[16], height_str[16];
            snprintf(width_str, sizeof(width_str), "%d", game_state->width);
            snprintf(height_str, sizeof(height_str), "%d", game_state->height);

            execl(player_paths[i], player_paths[i], width_str, height_str, NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]);
        players[i].pipe_fd = pipefd[0];
        players[i].pid = pid;
        game_state->players[i].pid = pid;

        fcntl(players[i].pipe_fd, F_SETFL, O_NONBLOCK);
    }

    return true;
}

bool create_view_process(const char *view_path) {
    if (view_path == NULL || strlen(view_path) == 0) {
        return true;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork view");
        return false;
    }

    if (pid == 0) {
        char width_str[16], height_str[16];
        snprintf(width_str, sizeof(width_str), "%d", game_state->width);
        snprintf(height_str, sizeof(height_str), "%d", game_state->height);

        execl(view_path, view_path, width_str, height_str, NULL);
        perror("execl view");
        exit(EXIT_FAILURE);
    }

    view_pid = pid;
    return true;
}

bool has_valid_moves(int player_idx) {
    int x = game_state->players[player_idx].x;
    int y = game_state->players[player_idx].y;
    int width = game_state->width;
    int height = game_state->height;

    for (int dir = 0; dir < 8; dir++) {
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

        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            char cell = game_state->board[ny * width + nx];
            if (cell > 0) {
                return true;
            }
        }
    }

    return false;
}

bool handle_player_move(int player_idx) {
    char buffer[64];
    ssize_t n = read(players[player_idx].pipe_fd, buffer, sizeof(buffer));

    if (n <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;
        }
        game_state->players[player_idx].is_blocked = true;
        return false;
    }

    unsigned char move = (unsigned char)buffer[0];
    int player_id = player_idx + 1;

    game_state->players[player_idx].invalid_moves++;

    int x = game_state->players[player_idx].x;
    int y = game_state->players[player_idx].y;
    int width = game_state->width;
    int height = game_state->height;
    int nx = x, ny = y;

    switch (move) {
        case DIR_UP: ny--; break;
        case DIR_UP_RIGHT: nx++; ny--; break;
        case DIR_RIGHT: nx++; break;
        case DIR_DOWN_RIGHT: nx++; ny++; break;
        case DIR_DOWN: ny++; break;
        case DIR_DOWN_LEFT: nx--; ny++; break;
        case DIR_LEFT: nx--; break;
        case DIR_UP_LEFT: nx--; ny--; break;
        default:
            sem_post(sem_player_ack[player_idx]);
            return true;
    }

    if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
        sem_post(sem_player_ack[player_idx]);
        return true;
    }

    char cell = game_state->board[ny * width + nx];
    if (cell <= 0) {
        sem_post(sem_player_ack[player_idx]);
        return true;
    }

    game_state->players[player_idx].score += cell;
    game_state->players[player_idx].valid_moves++;
    game_state->players[player_idx].invalid_moves--;

    game_state->board[y * width + x] = -player_id;
    game_state->board[ny * width + nx] = 0;
    game_state->players[player_idx].x = nx;
    game_state->players[player_idx].y = ny;

    game_state->players[player_idx].is_blocked = !has_valid_moves(player_idx);

    sem_post(sem_player_ack[player_idx]);
    return true;
}

void notify_view_and_wait(void) {
    if (view_pid <= 0) {
        return;
    }

    sem_post(sem_state_changed);
    sem_wait(sem_view_done);
}

bool all_players_blocked(void) {
    for (int i = 0; i < player_count; i++) {
        if (!game_state->players[i].is_blocked) {
            if (has_valid_moves(i)) {
                return false;
            }
            game_state->players[i].is_blocked = true;
        }
    }
    return true;
}

void finish_game(void) {
    game_state->is_finished = true;

    sem_post(sem_state_changed);

    for (int i = 0; i < player_count; i++) {
        int status;
        waitpid(players[i].pid, &status, 0);
        
        int exit_code = 0;
        int signal_num = 0;
        
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
            printf("Jugador %d termino con codigo de salida: %d, Score: %u\n", 
                   i + 1, exit_code, game_state->players[i].score);
        } else if (WIFSIGNALED(status)) {
            signal_num = WTERMSIG(status);
            printf("Jugador %d termino por senal: %d, Score: %u\n", 
                   i + 1, signal_num, game_state->players[i].score);
        }
    }

    if (view_pid > 0) {
        int status;
        waitpid(view_pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Vista termino con codigo de salida: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Vista termino por senal: %d\n", WTERMSIG(status));
        }
    }
}

void process_game_loop(int delay_ms, int timeout_s) {
    time_t last_valid_move = time(NULL);
    int round_robin_idx = 0;

    while (!game_state->is_finished) {
        if (all_players_blocked()) {
            break;
        }

        if (timeout_s > 0) {
            time_t now = time(NULL);
            if (now - last_valid_move > timeout_s) {
                break;
            }
        }

        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;

        for (int i = 0; i < player_count; i++) {
            if (!game_state->players[i].is_blocked && players[i].pipe_fd != -1) {
                FD_SET(players[i].pipe_fd, &read_fds);
                if (players[i].pipe_fd > max_fd) {
                    max_fd = players[i].pipe_fd;
                }
            }
        }

        if (max_fd == -1) {
            break;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ready <= 0) {
            continue;
        }

        bool valid_move_processed = false;

        for (int attempts = 0; attempts < player_count; attempts++) {
            int idx = (round_robin_idx + attempts) % player_count;

            if (game_state->players[idx].is_blocked) {
                continue;
            }

            if (!FD_ISSET(players[idx].pipe_fd, &read_fds)) {
                continue;
            }

            char buffer[64];
            ssize_t n = read(players[idx].pipe_fd, buffer, sizeof(buffer));

            if (n <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                game_state->players[idx].is_blocked = true;
                continue;
            }

            unsigned char move = (unsigned char)buffer[0];
            int player_id = idx + 1;

            game_state->players[idx].invalid_moves++;

            int x = game_state->players[idx].x;
            int y = game_state->players[idx].y;
            int width = game_state->width;
            int height = game_state->height;
            int nx = x, ny = y;

            switch (move) {
                case DIR_UP: ny--; break;
                case DIR_UP_RIGHT: nx++; ny--; break;
                case DIR_RIGHT: nx++; break;
                case DIR_DOWN_RIGHT: nx++; ny++; break;
                case DIR_DOWN: ny++; break;
                case DIR_DOWN_LEFT: nx--; ny++; break;
                case DIR_LEFT: nx--; break;
                case DIR_UP_LEFT: nx--; ny--; break;
                default:
                    sem_post(sem_player_ack[idx]);
                    valid_move_processed = true;
                    break;
            }

            if (!valid_move_processed && move < 8) {
                if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
                    sem_post(sem_player_ack[idx]);
                    valid_move_processed = true;
                } else {
                    char cell = game_state->board[ny * width + nx];
                    if (cell <= 0) {
                        sem_post(sem_player_ack[idx]);
                        valid_move_processed = true;
                    } else {
                        game_state->players[idx].score += cell;
                        game_state->players[idx].valid_moves++;
                        game_state->players[idx].invalid_moves--;

                        game_state->board[y * width + x] = -player_id;
                        game_state->board[ny * width + nx] = 0;
                        game_state->players[idx].x = nx;
                        game_state->players[idx].y = ny;

                        game_state->players[idx].is_blocked = !has_valid_moves(idx);

                        sem_post(sem_player_ack[idx]);
                        valid_move_processed = true;
                        last_valid_move = time(NULL);
                    }
                }
            }

            if (valid_move_processed) {
                round_robin_idx = (idx + 1) % player_count;

                notify_view_and_wait();

                if (delay_ms > 0) {
                    usleep(delay_ms * 1000);
                }

                break;
            }
        }
    }

    finish_game();
}

int main(int argc, char *argv[]) {
    int opt;
    
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    int delay_ms = DEFAULT_DELAY;
    int timeout = DEFAULT_TIMEOUT;
    int seed = time(NULL);
    char *view_path = NULL;
    char **player_paths = NULL;
    int num_players = 0;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
            case 'w':
                width = atoi(optarg);
                if (width < DEFAULT_WIDTH) width = DEFAULT_WIDTH;
                break;
            case 'h':
                height = atoi(optarg);
                if (height < DEFAULT_HEIGHT) height = DEFAULT_HEIGHT;
                break;
            case 'd':
                delay_ms = atoi(optarg);
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'v':
                view_path = optarg;
                break;
            case 'p':
                player_paths = &argv[optind - 1];
                while (optind < argc && argv[optind][0] != '-') {
                    num_players++;
                    optind++;
                }
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (num_players < MIN_PLAYERS || num_players > MAX_PLAYERS) {
        fprintf(stderr, "Error: Se requieren entre %d y %d jugadores\n", MIN_PLAYERS, MAX_PLAYERS);
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    player_count = num_players;

    if (!init_shared_memory(width, height)) {
        fprintf(stderr, "Error: No se pudo inicializar la memoria compartida\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (!init_sync_primitives()) {
        fprintf(stderr, "Error: No se pudieron inicializar los semaforos\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    init_board(seed);
    distribute_players();

    if (!create_player_processes(player_paths)) {
        fprintf(stderr, "Error: No se pudieron crear los procesos de jugadores\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (!create_view_process(view_path)) {
        fprintf(stderr, "Error: No se pudo crear el proceso de vista\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    process_game_loop(delay_ms, timeout);

    cleanup();
    return EXIT_SUCCESS;
}
