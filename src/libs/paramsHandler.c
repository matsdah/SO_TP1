#include <paramsHandler.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
** paramsHandler.c -> parser de argumentos con tabla de parámetros y manejo especial de -p múltiple.
*/

/* Valores default seguros para los parametros. */
Params defaultParams(){
    
    Params p = {
        .width = 10,
        .height = 10,
        .delay = 200,
        .timeout = 10,
        .seed = (size_t)time(NULL),
        .view = NULL,
        .players = { NULL },
        .playerCount = 0
    };
    return p;
}

static ParamDef gParameters[] = {
    {"-w", 1, handleWidth},
    {"-h", 1, handleHeight},
    {"-d", 1, handleDelay},
    {"-t", 1, handleTimeout},
    {"-s", 1, handleSeed},
    {"-v", 1, handleView},
    {"-p", 1, handlePlayers},
};

void freeParams(Params *config){
    if(config == NULL){
        return;
    }

    if(config->view != NULL){
        free(config->view);
        config->view = NULL;
    }

    for(size_t i = 0; i < config->playerCount && i < CANT_PLAYERS; i++){
        if(config->players[i] != NULL){
            free(config->players[i]);
            config->players[i] = NULL;
        }
    }

    for(size_t i = config->playerCount; i < CANT_PLAYERS; i++){
        config->players[i] = NULL;
    }

    config->playerCount = 0;
}

static int isFlag(const char *arg){
    return ((arg != NULL) && (arg[0] == '-') && (arg[1] != '\0'));
}

static int parseUnsignedShortInRange(const char *value, unsigned short *out, unsigned short min, unsigned short max, const char *fieldName){
    char *endptr = NULL;
    errno = 0;

    unsigned long parsed = strtoul(value, &endptr, 10);
    if(errno != 0 || endptr == value || *endptr != '\0'){
        fprintf(stderr, "Error: valor invalido para %s: '%s'.\n", fieldName, value);
        return 0;
    }

    if(parsed < min || parsed > max){
        if(min == 10 && max == USHRT_MAX){
            fprintf(stderr, "Error: %s debe ser mayor o igual a 10.\n", fieldName);
        }else{
            fprintf(stderr, "Error: %s debe estar entre %hu y %hu.\n", fieldName, min, max);
        }
        return 0;
    }

    *out = (unsigned short)parsed;
    return 1;
}

static int parseSizeTInRange(const char *value, size_t *out, size_t min, size_t max, const char *fieldName){
    char *endptr = NULL;
    errno = 0;

    unsigned long long parsed = strtoull(value, &endptr, 10);
    if(errno != 0 || endptr == value || *endptr != '\0'){
        fprintf(stderr, "Error: valor invalido para %s: '%s'.\n", fieldName, value);
        return 0;
    }

    if(parsed > (unsigned long long)SIZE_MAX){
        fprintf(stderr, "Error: valor fuera de rango para %s.\n", fieldName);
        return 0;
    }

    size_t parsedSize = (size_t)parsed;
    if(parsedSize < min || parsedSize > max){
        fprintf(stderr, "Error: %s fuera de rango.\n", fieldName);
        return 0;
    }

    *out = parsedSize;
    return 1;
}

const ParamDef *findParam(const char *arg, const ParamDef params[], int paramCount){

    /* Busca un parametro por su flag en el array de parametros. */
    for(int i = 0; i < paramCount; i++){
        if(strcmp(arg, params[i].flag) == 0){
            return &params[i];
        }
    }

    /* No se encontró el parametro. */
    return NULL;
}

int parseParams(int argc, char *argv[], Params *config){

    int paramCount = sizeof(gParameters) / sizeof(gParameters[0]);
    int playersFlagSeen = 0;

    /* Recorre todos los argumentos y procesa los parametros. */
    for(int i = 1; i < argc; i++){

        const ParamDef *param = findParam(argv[i], gParameters, paramCount);

        if(param == NULL){
            fprintf(stderr, "Error: parametro '%s' desconocido. \n", argv[i]);
            freeParams(config);
            return 0;
        }

        /* El flag -p puede recibir múltiples valores, así que se maneja de forma especial. */
        if(strcmp(param->flag, "-p") == 0){
            int consumed = 0;

            if(!playersFlagSeen){
                config->playerCount = 0;
                playersFlagSeen = 1;
            }

            while(i + 1 < argc && !isFlag(argv[i + 1])){
                if(!param->handler(argv[i + 1], config)){
                    freeParams(config);
                    return 0;
                }

                i++;
                consumed++;
            }

            if(consumed == 0){
                fprintf(stderr, "Error: parametro '-p' espera al menos un path de jugador.\n");
                freeParams(config);
                return 0;
            }

            continue;
        }

        /* Para los demás parametros, se maneja de forma normal. */
        if(param->expectsValue){
            if(i + 1 >= argc){
                fprintf(stderr, "Error: parametro '%s' espera un valor\n", argv[i]);
                freeParams(config);
                return 0;
            }

            const char *value = argv[i + 1];

            if(!param->handler(value, config)){
                freeParams(config);
                return 0;
            }

            i++;
        }else{
            if(!param->handler(NULL, config)){
                freeParams(config);
                return 0;
            }
        }
    }

    /* Valida la cantidad de jugadores. */
    if(config->playerCount < 1 || config->playerCount > CANT_PLAYERS){
        fprintf(stderr, "Error: cantidad de jugadores debe estar entre 1 y 9.\n");
        freeParams(config);
        return 0;
    }

    size_t totalCells = (size_t)config->width * (size_t)config->height;
    if(config->playerCount > totalCells){
        fprintf(stderr, "Error: la cantidad de jugadores (%zu) no puede superar las celdas del tablero (%zu).\n",
                config->playerCount, totalCells);
        freeParams(config);
        return 0;
    }

    return 1;
}

int handleWidth(const char *value, void *context){
    Params *cfg = (Params *)context;
    return parseUnsignedShortInRange(value, &cfg->width, 10, USHRT_MAX, "ancho");
}

int handleHeight(const char *value, void *context){
    Params *cfg = (Params *)context;
    return parseUnsignedShortInRange(value, &cfg->height, 10, USHRT_MAX, "alto");
}

int handleTimeout(const char *value, void *context){
    Params *cfg = (Params *)context;
    return parseSizeTInRange(value, &cfg->timeout, 0, SIZE_MAX, "timeout");
}

int handleDelay(const char *value, void *context){
    Params *cfg = (Params *)context;
    return parseSizeTInRange(value, &cfg->delay, 1, SIZE_MAX, "delay");
}

int handleSeed(const char *value, void *context){
    Params *cfg = (Params *)context;
    return parseSizeTInRange(value, &cfg->seed, 0, SIZE_MAX, "seed");
}

int handleView(const char *value, void *context) {
    Params *cfg = (Params *)context;
    size_t len = strlen(value);
    char *copy;

    if(cfg->view != NULL){
        free(cfg->view);
        cfg->view = NULL;
    }

    copy = malloc(len + 1);

    if(copy == NULL){
        perror("Error reservando memoria para el path de la vista");
        return 0;
    }

    memcpy(copy, value, len + 1);
    cfg->view = copy;

    return 1;
}

int handlePlayers(const char *value, void *context){
    Params *cfg = (Params *)context;
    size_t len = strlen(value);
    char *copy;

    if(cfg->playerCount >= CANT_PLAYERS){
        fprintf(stderr, "Error: cantidad maxima de jugadores es 9.\n");
        return 0;
    }

    copy = malloc(len + 1);
    if(copy == NULL){
        perror("Error reservando memoria para el path del jugador");
        return 0;
    }

    memcpy(copy, value, len + 1);
    cfg->players[cfg->playerCount] = copy;
    cfg->playerCount++;

    return 1;
}
