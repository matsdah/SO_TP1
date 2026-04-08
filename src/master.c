#include <gameLogic.h>

/* Variables globales para el signal handler */
static volatile sig_atomic_t gInterrupted = 0;
static PlayerProcess *gPlayerProcesses = NULL;
static int gPlayerCount = 0;
static GameState *gGameState = NULL;
static SyncData *gGameSync = NULL;
static int gStateFd = -1;
static int gSyncFd = -1;
static size_t gWidth = 0;
static size_t gHeight = 0;
static pid_t gViewPid = -1;

static void signalHandler(int signum) {
    (void)signum;
    gInterrupted = 1;

    if (gGameState != NULL) {
        gGameState->gameOver = 1;
    }
}

int main(int argc, char *argv[]) {
    Params params = defaultParams();

    if (!parseParams(argc, argv, &params)) {
        fprintf(stderr, "Uso minimo: %s -p <player_path> [-v <view_path>] [-w <width>] [-h <height>]\n", argv[0]);
        return 1;
    }

    /* Configurar manejador de senales */
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    /* Crear memoria compartida y semaforos */
    int stateFd = -1;
    int syncFd = -1;
    GameState *gameState = NULL;
    SyncData *gameSync = NULL;

    if (stateCreate(&stateFd, &gameState, params.width, params.height) == -1) {
        perror("stateCreate");
        return 1;
    }

    if (syncCreate(&syncFd, &gameSync) == -1) {
        perror("syncCreate");
        stateClose(stateFd, gameState, params.width, params.height);
        stateUnlink();
        return 1;
    }

    if (syncInit(gameSync, (unsigned int)params.playerCount) == -1) {
        perror("syncInit");
        syncClose(syncFd, gameSync);
        syncUnlink();
        stateClose(stateFd, gameState, params.width, params.height);
        stateUnlink();
        return 1;
    }

    /* Inicializar estado del juego */
    gameState->width = params.width;
    gameState->height = params.height;
    gameState->playerCount = params.playerCount;
    gameState->gameOver = false;

    initializeBoard(gameState, params.seed);
    placePlayers(gameState);

    /* Crear procesos de jugadores */
    PlayerProcess playerProcesses[CANT_PLAYERS];

    if (spawnPlayers(&params, playerProcesses, gameState) == -1) {
        cleanup(playerProcesses, params.playerCount, gameState, gameSync, 
                stateFd, syncFd, params.width, params.height, -1);
        return 1;
    }

    /* Crear proceso de visualizacion */
    pid_t viewPid = -1;

    if (params.view != NULL) {
        viewPid = fork();

        if (viewPid == 0) {
            char widthStr[16];
            char heightStr[16];

            snprintf(widthStr, sizeof(widthStr), "%zu", params.width);
            snprintf(heightStr, sizeof(heightStr), "%zu", params.height);

            execl(params.view, params.view, widthStr, heightStr, NULL);
            perror("execl view");
            exit(1);

        } else if (viewPid < 0) {
            perror("fork view");
        }
    }

    /* Configurar variables globales para signal handler */
    gPlayerProcesses = playerProcesses;
    gPlayerCount = params.playerCount;
    gGameState = gameState;
    gGameSync = gameSync;
    gStateFd = stateFd;
    gSyncFd = syncFd;
    gWidth = params.width;
    gHeight = params.height;
    gViewPid = viewPid;

    /* Notificar estado inicial a la vista */
    if (viewPid > 0) {
        notifyView(gameSync);
    }

    /* Loop principal */
    time_t startTime = time(NULL);
    int currentPlayer = 0;
    fd_set readFds;
    struct timeval tv;

    while (!gameState->gameOver && !gInterrupted) {
        checkGameOver(gameState, startTime, params.timeout);

        if (gameState->gameOver) {
            break;
        }

        /* Round-robin: leer del jugador actual */
        FD_ZERO(&readFds);
        FD_SET(playerProcesses[currentPlayer].pipeFd, &readFds);

        tv.tv_sec = 0;
        tv.tv_usec = params.delay * 1000;

        int ret = select(playerProcesses[currentPlayer].pipeFd + 1, &readFds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(playerProcesses[currentPlayer].pipeFd, &readFds)) {
            unsigned char direction;
            ssize_t bytesRead = read(playerProcesses[currentPlayer].pipeFd, &direction, 1);

            if (bytesRead == 1) {
                /* Adquirir writer lock */
                sem_wait(&gameSync->masterMutex);
                sem_wait(&gameSync->stateMutex);

                /* Validar y aplicar movimiento */
                if (validateMove(gameState, currentPlayer, direction)) {
                    applyMove(gameState, currentPlayer, direction);
                    gameState->players[currentPlayer].validMoves++;
                    startTime = time(NULL);
                } else {
                    gameState->players[currentPlayer].invalidMoves++;
                }

                /* Liberar writer lock */
                sem_post(&gameSync->stateMutex);
                sem_post(&gameSync->masterMutex);

                /* Notificar vista */
                if (viewPid > 0) {
                    notifyView(gameSync);
                }

                /* Enviar ACK al jugador */
                sem_post(&gameSync->playerSem[currentPlayer]);
            }
        }

        currentPlayer = (currentPlayer + 1) % (int)params.playerCount;
        usleep(params.delay * 1000);
    }

    if (gInterrupted) {
        fprintf(stderr, "\n\nInterrupted by signal, cleaning up...\n");
    } else {
        printResults(gameState);
    }

    cleanup(playerProcesses, (int)params.playerCount, gameState, gameSync, 
            stateFd, syncFd, params.width, params.height, viewPid);

    /* Esperar a que termine la vista */
    if (viewPid > 0) {
        int viewStatus;
        waitpid(viewPid, &viewStatus, 0);

        if (WIFEXITED(viewStatus)) {
            printf("View (PID %d): exited with status %d\n",
                   viewPid, WEXITSTATUS(viewStatus));
        } else if (WIFSIGNALED(viewStatus)) {
            printf("View (PID %d): killed by signal %d\n",
                   viewPid, WTERMSIG(viewStatus));
        }
    }

    return 0;
}
