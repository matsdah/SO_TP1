#include <viewRender.h>
#include <shmState.h>
#include <shmSync.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

static volatile sig_atomic_t gInterrupted = 0;

static void signalHandler(int sig){
    (void)sig;
    gInterrupted = 1;
}

static int parseDimension(const char *value, size_t *out, const char *name){
    char *endptr = NULL;
    errno = 0;
    unsigned long parsed = strtoul(value, &endptr, 10);

    if(errno != 0 || endptr == value || *endptr != '\0' || parsed == 0 || parsed > USHRT_MAX){
        fprintf(stderr, "Error: %s invalido '%s'. Debe estar entre 1 y %u.\n", name, value, USHRT_MAX);
        return -1;
    }

    *out = (size_t)parsed;
    return 0;
}

/* 
** vista.c -> Renderizar el estado del juego en la terminal, espera notificaciones del master 
** para actualizar la vista, toma escritura exclusiva para mostrar el tablero y las estadisticas 
** de cada jugador. Al final muestra el tablero final y las estadisticas finales.
*/

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        return 1;
    }

    size_t width = 0;
    size_t height = 0;

    if(parseDimension(argv[1], &width, "width") == -1 || parseDimension(argv[2], &height, "height") == -1){
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGINT, &sa, NULL) == -1){
        perror("Error en sigaction");
        return 1;
    }

    int syncFd = -1;        /* File descriptor de sincronizacion del juego. */
    int stateFd = -1;       /* File descriptor de estado del juego. */

    SyncData *sync = NULL;      /* Puntero a la estructura de sincronizacion del juego. */
    GameState *state = NULL;    /* Puntero a la estructura de estado del juego. */
    int shouldPrintFinalOnMainScreen = 0;

    if(syncOpen(&syncFd, &sync) == -1){
        /* Fallo la sincronizacion, muestro mensaje de error. */
        perror("Error de sincronizacion - syncOpen");
        return 1;
    }

    if(stateOpen(&stateFd, &state, width, height) == -1){
        /* Fallo la apertura del estado del juego, muestro mensaje de error. */
        perror("Error de estado - stateOpen");
        syncClose(syncFd, sync);
        return 1;
    }

    enterAlternateScreen();

    /* Loop principal de RENDERIZADO. */
    while(1){
        if(gInterrupted){
            break;
        }

        int waitRes;
        do{
            waitRes = sem_wait(&sync->printNeeded);
        }while(waitRes == -1 && errno == EINTR && !gInterrupted);

        if(gInterrupted){
            break;
        }

        if(waitRes == -1){
            perror("Error en sem_wait printNeeded");
            break;
        }

        if(acquireReadLock(sync) == -1){
            break;
        }

        if(state->gameOver){
            /* Si termina el juego, muestro el tablero final y las estadisticas. */
            shouldPrintFinalOnMainScreen = 1;
            clearScreen();
            printView(state);
            printStats(state);
            if(releaseReadLock(sync) == -1){
                break;
            }
            if(sem_post(&sync->renderDone) == -1){
                perror("Error en sem_post renderDone");
                break;
            }
            break;      /* Salgo del loop principal. */
        }

        /* Por cada iteración del loop principal, actualizo la vista del juego junto con estadisticas. */
        clearScreen();
        printView(state);
        printStats(state);

        if(releaseReadLock(sync) == -1){
            break;
        }

        if(sem_post(&sync->renderDone) == -1){
            perror("Error en sem_post renderDone");
            break;
        }
    }

    leaveAlternateScreen();
    if(shouldPrintFinalOnMainScreen){
        clearScreen();
        printView(state);
        printStats(state);
    }

    /* Cierro los recursos de sincronizacion (semaforos, pipes). */
    if(syncClose(syncFd, sync) == -1){
        perror("Error en syncClose");
    }

    /* Cierro los recursos de estado del juego. */
    if(stateClose(stateFd, state, width, height) == -1){
        perror("Error en stateClose");
    }

    return 0;
}
