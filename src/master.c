#include <gameLogic.h>
#include <paramsHandler.h>
#include <shmState.h>
#include <shmSync.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>

/* 
** master.c -> parsea argumentos, crea memoria compartida, inicializa semáforos, 
** spawnea jugadores-vista, ejecuta el loop principal del juego, y hace cleanup al finalizar.
*/

/* Variables globales para el signal handler */
static volatile sig_atomic_t gInterrupted = 0;  /* Variable para indicar si se recibió una señal de interrupción. */
static GameState *gGameState = NULL;            /* Estado del juego. */

static void signalHandler(int sig){
    gInterrupted = 1;       /* Indica que se recibió una señal de interrupción. */
}

int main(int argc, char *argv[]){
    /* Asigno parámetros por defecto por si no recibo parametros luego. */
    Params params = defaultParams();        

    /* Parseo parametros (si aplica). */
    if(!parseParams(argc, argv, &params)){
        fprintf(stderr, "Uso minimo: %s -p <player_path> [-v <view_path>] [-w <width>] [-h <height>]\n", argv[0]);
        return 1;
    }

    struct sigaction sa;            /* Crear manejador de señales. */      
    sa.sa_handler = signalHandler;  
    sigemptyset(&sa.sa_mask);       /* Inicializar conjunto de señales. */
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1){
        /* Manejar error de sigaction por flags mal usados, errores de inicilización, etc. */
        perror("Error de sigaction.");
        return 1;
    }

    /* Crear memoria compartida y semaforos */
    int stateFd = -1;
    int syncFd = -1;
    GameState *gameState = NULL;
    SyncData *gameSync = NULL;

    if(stateCreate(&stateFd, &gameState, params.width, params.height) == -1){
        perror("Error en stateCreate.");
        return 1;
    }

    if(syncCreate(&syncFd, &gameSync) == -1){
        perror("Error en syncCreate.");
        stateClose(stateFd, gameState, params.width, params.height);
        stateUnlink();
        return 1;
    }

    if(syncInit(gameSync, params.playerCount) == -1){
        perror("Error en syncInit.");
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
    PlayerProcess playerProcesses[gameState->playerCount];

    for(int i = 0; i < gameState->playerCount; i++){
        playerProcesses[i].pid = -1;
        playerProcesses[i].pipeFd = -1;
    }
    
    if(spawnPlayers(&params, playerProcesses, gameState) == -1){
        cleanup(playerProcesses, params.playerCount, gameState, gameSync, stateFd, syncFd, params.width, params.height, -1, 0);
        return 1;
    }

    /* Crear proceso de visualizacion */
    pid_t viewPid = -1;

    if(params.view != NULL){
        viewPid = fork();

        if(viewPid == 0){
            /* Strings para almacenar el ancho y alto del tablero. */
            char widthStr[16];      
            char heightStr[16];

            snprintf(widthStr, sizeof(widthStr), "%zu", params.width);
            snprintf(heightStr, sizeof(heightStr), "%zu", params.height);

            execl(params.view, params.view, widthStr, heightStr, NULL);
            perror("execl view");
            exit(1);

        }else{
            if(viewPid < 0){
                perror("Error en la vista de fork.");
            }
        }
    }

    /* Configurar variables globales para signal handler */
    gGameState = gameState;

    /* Notificar estado inicial a la vista */
    if(viewPid > 0){
        notifyView(gameSync);
    }

    /* Loop principal */
    time_t startTime = time(NULL);
    int currentPlayer = 0;
    fd_set readFds;             /* Conjunto de descriptores de archivo. */
    struct timeval tv;          /* Estructura para el timeout. */

    while(!gameState->gameOver && !gInterrupted){
        /* En cada iteracion, chequeo si el juego finalizó. */
        checkGameOver(gameState, startTime, params.timeout);
        
        if(gInterrupted){
            gameState ->gameOver = true;    /* Si se recibió una señal de interrupción, marco el juego como terminado. */
        }
        if(gameState->gameOver){
            break;
        }

        /* Round-robin: leer del jugador actual */
        FD_ZERO(&readFds);
        FD_SET(playerProcesses[currentPlayer].pipeFd, &readFds);

        tv.tv_sec = 0;
        tv.tv_usec = params.delay * 1000;

        int ret = select(playerProcesses[currentPlayer].pipeFd + 1, &readFds, NULL, NULL, &tv);

        if((ret > 0) && FD_ISSET(playerProcesses[currentPlayer].pipeFd, &readFds)){
            unsigned char direction;        /* Dirección recibida del jugador. */

            /* Leer dirección del jugador. */
            ssize_t bytesRead = read(playerProcesses[currentPlayer].pipeFd, &direction, 1);

            if(bytesRead == 1){
                /* Adquirir escritura exclusiva. */
                sem_wait(&gameSync->masterMutex);
                sem_wait(&gameSync->stateMutex);

                /* Validar y aplicar movimiento */
                if(validateMove(gameState, currentPlayer, direction)){
                    applyMove(gameState, currentPlayer, direction);
                    gameState->players[currentPlayer].validMoves++;
                    startTime = time(NULL);     /* Actualizar el tiempo de inicio. */
                }else{
                    gameState->players[currentPlayer].invalidMoves++;
                }

                /* Liberar escritura exclusiva. */
                sem_post(&gameSync->stateMutex);
                sem_post(&gameSync->masterMutex);

                /* Notificar vista. */
                if(viewPid > 0){
                    notifyView(gameSync);
                }

                /* Desbloqueo al jugador actual si estaba esperando permiso. 
                ** Evito que el jugador siga ejecutándose antes de que el master 
                ** termine de procesar su movimiento. */
                sem_post(&gameSync->playerSem[currentPlayer]);
            }
        }

        /* Avanzo al siguiente jugador de forma ciruclar. */
        currentPlayer = (currentPlayer + 1) % params.playerCount;

        /* Agregamos un pequeño delay para que las interaciones sean visibles. */
        usleep(params.delay * 1000);
    }

    if(gInterrupted){
        fprintf(stderr, "\n\nInterrumpido por señal, borrando...\n");   /* por ej.: CTRL + C*/
    }else{
        printResults(gameState);
    }

    cleanup(playerProcesses, (int)params.playerCount, gameState, gameSync, stateFd, syncFd, params.width, params.height, viewPid, gInterrupted);

    /* Esperar a que termine la vista */
    if(viewPid > 0){
        int viewStatus;

        waitpid(viewPid, &viewStatus, 0);

        if(WIFEXITED(viewStatus)){
            printf("Vista (PID: %d): salió con estado: %d\n", viewPid, WEXITSTATUS(viewStatus));
        }else{
            if(WIFSIGNALED(viewStatus)){
                printf("Vista (PID: %d): abortado por señal: %d\n", viewPid, WTERMSIG(viewStatus));
            }
        }
    }

    return 0;
}