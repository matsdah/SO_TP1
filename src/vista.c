#include <viewRender.h>
#include <shmState.h>
#include <shmSync.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

/* 
** vista.c -> Renderizar el estado del juego en la terminal, espera notificaciones del master 
** para actualizar la vista, toma escritura exclusiva para mostrar el tablero y las estadisticas 
** de cada jugador. Al final muestra el tablero final y las estadisticas finales.
*/

int main(int argc, char *argv[]){
    if(argc < 3){
        /* Muestro mensaje de error por parametros incorrectos. */
        fprintf(stderr, "Uso minimo de los parámetros: %s <width> <height>\n", argv[0]);
        return 1;
    }

    size_t width = atoi(argv[1]);       /* Paso el ancho como argumento a entero. */
    size_t height = atoi(argv[2]);      /* Paso la altura como argumento a entero. */

    int syncFd = -1;        /* File descriptor de sincronizacion del juego. */
    int stateFd = -1;       /* File descriptor de estado del juego. */

    SyncData *sync = NULL;      /* Puntero a la estructura de sincronizacion del juego. */
    GameState *state = NULL;    /* Puntero a la estructura de estado del juego. */

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

    /* Loop principal de RENDERIZADO. */
    while(1){
        sem_wait(&sync->printNeeded);

        acquireReadLock(sync);

        if(state->gameOver){
            /* Si termina el juego, muestro el tablero final y las estadisticas. */
            clearScreen();
            printView(state);
            printStats(state);
            releaseReadLock(sync);
            sem_post(&sync->renderDone);
            break;      /* Salgo del loop principal. */
        }

        /* Por cada iteración del loop principal, actualizo la vista del juego junto con estadisticas. */
        clearScreen();
        printView(state);
        printStats(state);

        releaseReadLock(sync);

        sem_post(&sync->renderDone); 
    }

    /* Cierro los recursos de sincronizacion (semaforos, pipes). */
    syncClose(syncFd, sync);

    /* Cierro los recursos de estado del juego. */
    stateClose(stateFd, state, width, height);

    return 0;
}
