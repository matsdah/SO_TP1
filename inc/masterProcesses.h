#include <structures.h>

void setInitialGameState(GameState * gameState, Parameters params);


void setInitialPlayersState(GameState * gameState, char * players[]);


void fillBoard(int width, int height, int * board);


void setInitialTimeouts(int timeoutValue);


void setInitialPlayersPosition(GameState * gameState);
