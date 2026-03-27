#include "shmCommon.h"
#include "structures.h"

semaphoresStatus * createGameSync(){
    return (semaphoresStatus *) createSHM("/gameSync", sizeof(semaphoresStatus), 0666);
}