#include <gameLogic.h>
#include <paramsHandler.h>
#include <shmState.h>
#include <shmSync.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* 
** master.c -> parsea argumentos, crea memoria compartida, inicializa semáforos, 
** spawnea jugadores-vista, ejecuta el loop principal del juego, y hace cleanup al finalizar.
*/

/* Variable global para indicar si se recibió una señal de interrupción. */
static volatile sig_atomic_t gInterrupted = 0;  

static void signalHandler(int sig){
    (void)sig;
    gInterrupted = 1;       /* Indica que se recibió una señal de interrupción. */
}

int main(int argc, char *argv[]){
    /* Asigno parámetros por defecto por si no recibo parametros luego. */
    Params params = defaultParams();        

    /* Parseo parametros (si aplica). */
    if(!parseParams(argc, argv, &params)){
        fprintf(stderr, "Uso: %s -p <player_path> [<player_path> ...] [-v <view_path>] [-w <width>] [-h <height>] [-d <delay_ms>] [-t <timeout_s>] [-s <seed>]\n", argv[0]);
        return 1;
    }

    struct sigaction sa;            /* Crear manejador de señales. */      
    sa.sa_handler = signalHandler;  
    sigemptyset(&sa.sa_mask);       /* Inicializar conjunto de señales. */
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1){
        /* Manejar error de sigaction por flags mal usados, errores de inicilización, etc. */
        perror("Error de sigaction.");
        freeParams(&params);
        return 1;
    }

    int stateFd = -1;
    int syncFd = -1;

    GameState *gameState = NULL;
    SyncData *gameSync = NULL;

    /* Crea las memorias compartidas (gameState y gameSync). */
    if(setupGameResources(&params, &stateFd, &gameState, &syncFd, &gameSync) == -1){
        freeParams(&params);
        return 1;
    }

    /* Inicializa gameState. */
    setupInitialGameState(gameState, &params);

    PlayerProcess playerProcesses[CANT_PLAYERS];

    /* Fork de cada jugador en el array playerProcesses. */
    initPlayerProcesses(playerProcesses, (int)gameState->playerCount);
    
    if(spawnPlayers(&params, playerProcesses, gameState) == -1){
        cleanup(playerProcesses, params.playerCount, gameState, gameSync, stateFd, syncFd, params.width, params.height, -1, 0);
        freeParams(&params);
        return 1;
    }

    pid_t viewPid = -1;
    int viewReady = 0;

    if(spawnViewProcess(&params, &viewPid, &viewReady) == -1){
        cleanup(playerProcesses, params.playerCount, gameState, gameSync, stateFd, syncFd, params.width, params.height, -1, 0);
        freeParams(&params);
        return 1;
    }

    /* Notificar estado inicial a la vista */
    if(viewReady){
        if(notifyView(gameSync) == -1){
            viewReady = 0;
        }
    }

    runMasterLoop(gameState, gameSync, playerProcesses, &params, &viewReady, &gInterrupted);

    if(viewReady){
        /* Fuerza un último render antes de esperar a la vista. */
        (void)notifyView(gameSync);
    }

    int viewWaitResult = waitViewProcess(viewPid);
    viewPid = -1;

    if(gInterrupted){
        fprintf(stderr, "\nInterrumpido por señal, borrando...\n");   /* por ej.: CTRL + C*/
    }else{
        printResults(gameState);
    }

    cleanup(playerProcesses, (int)params.playerCount, gameState, gameSync, stateFd, syncFd, params.width, params.height, viewPid, gInterrupted);
    freeParams(&params);

    return (viewWaitResult == -1) ? 1 : 0;
}