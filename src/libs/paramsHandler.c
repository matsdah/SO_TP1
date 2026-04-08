#include <paramsHandler.h>

/* Valores default seguros para los parametros. */
Params defaultParams(){
    Params p = {
        .width = 10,
        .height = 10,
        .delay = 200,
        .timeout = 10,
        .seed = 67,
        .view = NULL,
        .players = { NULL },
        .playerCount = 0
    };
    return p;
}

static ParamDef gParameters[] = {
    {"-w", 1, handleWidth,   "Set width  (example: -w 20)"},
    {"-h", 1, handleHeight,  "Set height (example: -h 10)"},
    {"-d", 1, handleDelay,   "Set delay (example: -d 100)"},
    {"-t", 1, handleTimeout, "Set timeout (example: -t 15)"},
    {"-s", 1, handleSeed,    "Set seed (example: -s 67)"},
    {"-v", 1, handleView,    "Set view path (example: -v ./vista)"},
    {"-p", 1, handlePlayers, "Set players path (example: -p ./player ./bin/player2)"},
};

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
                    return 0;
                }

                i++;
                consumed++;
            }

            if(consumed == 0){
                fprintf(stderr, "Error: parametro '-p' espera al menos un path de jugador.\n");
                return 0;
            }

            continue;
        }

        /* Para los demás parametros, se maneja de forma normal. */
        if(param->expectsValue){
            if(i + 1 >= argc){
                fprintf(stderr, "Error: parametro '%s' espera un valor\n", argv[i]);
                return 0;
            }

            const char *value = argv[i + 1];

            if(!param->handler(value, config)){
                return 0;
            }

            i++;
        }else{
            if(!param->handler(NULL, config)){
                return 0;
            }
        }
    }

    /* Valida la cantidad de jugadores. */
    if(config->playerCount < 1 || config->playerCount > 9){
        fprintf(stderr, "Error: cantidad de jugadores debe estar entre 1 y 9.\n");
        return 0;
    }

    return 1;
}

int handleWidth(const char *value, void *context){
    Params *cfg = (Params *)context;
    cfg->width = atoi(value);

    return 1;
}

int handleHeight(const char *value, void *context){
    Params *cfg = (Params *)context;
    cfg->height = atoi(value);

    return 1;
}

int handleTimeout(const char *value, void *context){
    Params *cfg = (Params *)context;
    cfg->timeout = atoi(value);

    return 1;
}

int handleDelay(const char *value, void *context){
    Params *cfg = (Params *)context;
    cfg->delay = atoi(value);

    return 1;
}

int handleSeed(const char *value, void *context){
    Params *cfg = (Params *)context;
    cfg->seed = atoi(value);

    return 1;
}

int handleView(const char *value, void *context) {
    Params *cfg = (Params *)context;
    size_t len = strlen(value);
    char *copy = malloc(len + 1);

    if(copy == NULL){
        fprintf(stderr, "Error: no hay memoria suficiente para el path de la vista.\n");
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

    if(cfg->playerCount >= 9){
        fprintf(stderr, "Error: cantidad maxima de jugadores es 9.\n");
        return 0;
    }

    copy = malloc(len + 1);
    if(copy == NULL){
        fprintf(stderr, "Error: no hay memoria suficiente para el path del jugador.\n");
        return 0;
    }

    memcpy(copy, value, len + 1);
    cfg->players[cfg->playerCount] = copy;
    cfg->playerCount++;

    return 1;
}

int isFlag(const char *arg){
    return ((arg != NULL) && (arg[0] == '-') && (arg[1] != '\0'));
}