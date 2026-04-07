#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paramsHandler.h>
#include <structures.h>

static int isFlag(const char *arg);

static ParamDef gParameters[] = {
    {"-w", 1, handleWidth,   "Set width  (example: -w 20)"},
    {"-h", 1, handleHeight,  "Set height (example: -h 10)"},
    {"-d", 1, handleDelay,   "Set delay (example: -d 100)"},
    {"-t", 1, handleTimeout, "Set timeout (example: -t 15)"},
    {"-s", 1, handleSeed,    "Set seed (example: -s 67)"},
    {"-v", 1, handleView,    "Set view path (example: -v ./vista)"},
    {"-p", 1, handlePlayers, "Set players path (example: -p ./player ./bin/player2)"},
};

Params defaultParams(void) {
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

const ParamDef *findParam(const char *arg, const ParamDef params[], int paramCount) {
    for (int i = 0; i < paramCount; i++) {
        if (strcmp(arg, params[i].flag) == 0) {
            return &params[i];
        }
    }
    return NULL;
}

int parseParams(int argc, char *argv[], Params *config) {
    int paramCount = sizeof(gParameters) / sizeof(gParameters[0]);
    int playersFlagSeen = 0;

    for (int i = 1; i < argc; i++) {
        const ParamDef *param = findParam(argv[i], gParameters, paramCount);

        if (param == NULL) {
            fprintf(stderr, "Error: unknown parameter '%s'\n", argv[i]);
            return 0;
        }

        if (strcmp(param->flag, "-p") == 0) {
            int consumed = 0;

            if (!playersFlagSeen) {
                config->playerCount = 0;
                playersFlagSeen = 1;
            }

            while (i + 1 < argc && !isFlag(argv[i + 1])) {
                if (!param->handler(argv[i + 1], config)) {
                    return 0;
                }
                i++;
                consumed++;
            }

            if (consumed == 0) {
                fprintf(stderr, "Error: parameter '-p' expects at least one player path\n");
                return 0;
            }

            continue;
        }

        if (param->expectsValue) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: parameter '%s' expects a value\n", argv[i]);
                return 0;
            }

            const char *value = argv[i + 1];

            if (!param->handler(value, config)) {
                return 0;
            }

            i++;
        } else {
            if (!param->handler(NULL, config)) {
                return 0;
            }
        }
    }

    if (config->playerCount < 1 || config->playerCount > 9) {
        fprintf(stderr, "Error: amount of players must be between 1 and 9\n");
        return 0;
    }

    return 1;
}

int handleWidth(const char *value, void *context) {
    Params *cfg = (Params *)context;
    cfg->width = atoi(value);
    return 1;
}

int handleHeight(const char *value, void *context) {
    Params *cfg = (Params *)context;
    cfg->height = atoi(value);
    return 1;
}

int handleTimeout(const char *value, void *context) {
    Params *cfg = (Params *)context;
    cfg->timeout = atoi(value);
    return 1;
}

int handleDelay(const char *value, void *context) {
    Params *cfg = (Params *)context;
    cfg->delay = atoi(value);
    return 1;
}

int handleSeed(const char *value, void *context) {
    Params *cfg = (Params *)context;
    cfg->seed = atoi(value);
    return 1;
}

int handleView(const char *value, void *context) {
    Params *cfg = (Params *)context;
    size_t len = strlen(value);
    char *copy = malloc(len + 1);
    if (copy == NULL) {
        fprintf(stderr, "Error: not enough memory for view path\n");
        return 0;
    }

    memcpy(copy, value, len + 1);
    cfg->view = copy;
    return 1;
}

int handlePlayers(const char *value, void *context) {
    Params *cfg = (Params *)context;
    size_t len = strlen(value);
    char *copy;

    if (cfg->playerCount >= 9) {
        fprintf(stderr, "Error: maximum number of players is 9\n");
        return 0;
    }

    copy = malloc(len + 1);
    if (copy == NULL) {
        fprintf(stderr, "Error: not enough memory for player path\n");
        return 0;
    }

    memcpy(copy, value, len + 1);
    cfg->players[cfg->playerCount] = copy;
    cfg->playerCount++;
    return 1;
}

static int isFlag(const char *arg) {
    return arg != NULL && arg[0] == '-' && arg[1] != '\0';
}
