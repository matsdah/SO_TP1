/* Aca van todos las implementaciones de las funciones del master.c. */

#include <time.h>
#include <math.h>
#include <masterProcesses.h>
#include <structures.h>

static int timeout = 0;

static int timeouts[CANT_PLAYERS] = {0};

void setInitialGameState(GameState * gameState, Parameters params){

    gameState->width = params.width;
    gameState->height = params.height;
    gameState->gameOver = false;
    gameState->playersCount = params.amount_players;
    srand(params.seed);
    fillBoard(gameState->width, gameState->height, gameState->board);
    setInicialPlayersPosition(gameState);
    setInitialPlayersState(gameState, params.players);
    setInitialTimeouts(params.timeout);

}


void setInitialPlayersState(GameState * gameState, char * players[]){

    for (int i = 0; i < gameState->playersCount; i++){
        char name[150];
        strcpy(name, players[i]);
        strcpy(gameState->players[i].playerName, basename(name));
        gameState->players[i].points = 0;
        gameState->players[i].invalidMovements = 0;
        gameState->players[i].validMovements = 0;
        gameState->players[i].playerPID = 0;
        gameState->players[i].valid = 0;
        // BOARD_AT_PLAYER(gameState, i) = i * (-1);
    }

}


void fillBoard(int width, int height, int * board){

    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            board[i * width + j] = randInt(1, 9);
        }
    }

}


void setInitialTimeouts(int timeoutValue){
    timeout = timeoutValue;

    time_t currentTime = time(NULL);

    for(int i = 0; i < CANT_PLAYERS; i++){
        timeouts[i] = current_time;
    }
}


void setInicialPlayersPosition(GameState * gameState){

    double h = (gameState->height - 1) / 2.0;
    double k = (gameState->width - 1) / 2.0;

    double a = h * 0.8;
    double b = k * 0.8;

    for(int i = 0; i < gameState->playersCount; i++){

        double theta = 2.0 * M_PI * i / gameState->playersCount;

        double xf = k + b * sin(theta);
        double yf = h + a * cos(theta);

        gameState->players[i].x = (int) round(xf);
        gameState->players[i].y = (int) round(yf);
    }
}
