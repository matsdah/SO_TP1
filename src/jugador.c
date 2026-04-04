#include "../include/structures.h"

int main(int argc, char const *argv[]){
    
    size_t heigth;
    size_t width;

    heigth = atoi(argv[1]);
    width = atoi(argv[2]);

    semaphoresStatus * gameSync = get_game_sync();
    //chequear el size
    GameState * gameState = get_game_state(GAME_STATUS_SIZE(game_state, width, height));
    
    return 0;
}