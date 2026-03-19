#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <semaphore.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>

#define MAX_PLAYERS 9
#define MIN_PLAYERS 1
#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

#define SHM_STATE_NAME "/game_state"
#define SHM_SYNC_NAME "/game_sync"

#define SEM_STATE_CHANGED "/sem_state_changed"
#define SEM_VIEW_DONE "/sem_view_done"
#define SEM_WRITE_QUEUE "/sem_write_queue"
#define SEM_STATE_MUTEX "/sem_state_mutex"
#define SEM_READ_COUNT_MUTEX "/sem_read_count_mutex"
#define SEM_PLAYER_ACK_PREFIX "/sem_player_ack_"

#define DIR_UP 0
#define DIR_UP_RIGHT 1
#define DIR_RIGHT 2
#define DIR_DOWN_RIGHT 3
#define DIR_DOWN 4
#define DIR_DOWN_LEFT 5
#define DIR_LEFT 6
#define DIR_UP_LEFT 7

typedef struct {
    char name[16];
    unsigned int score;
    unsigned int invalid_moves;
    unsigned int valid_moves;
    unsigned short x, y;
    pid_t pid;
    bool is_blocked;
} player_t;

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned char player_count;
    player_t players[MAX_PLAYERS];
    bool is_finished;
    char board[];
} game_state_t;

typedef struct {
    unsigned int readers_count;
} sync_data_t;

#endif
