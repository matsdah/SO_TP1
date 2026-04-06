#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"


int handle_width(const char *value, void *context);
int handle_height(const char *value, void *context);
int handle_delay(const char *value, void *context);
int handle_timeout(const char *value, void *context);
int handle_seed(const char *value, void *context);
int handle_view(const char *value, void *context);
int handle_players(const char *value, void *context);
static int is_flag(const char *arg);

Parameter parameters[] = {
    {"-w", 1, handle_width,   "Set width  (example: -w 20)"},
    {"-h", 1, handle_height,  "Set height (example: -h 10)"},
    {"-d", 1, handle_delay,  "Set delay (example: -d 100)"},
    {"-t", 1, handle_timeout,  "Set timeout (example: -t 15)"},
    {"-s", 1, handle_seed,  "Set seed (example: -s 67)"},
    {"-v", 1, handle_view,  "Set view path (example: -v ./vista)"},
    {"-p", 1, handle_players,  "Set players path (example: -p ./player ./bin/player2)"},
};

Parameters default_parameters(void){
    Parameters p = {
        .width = 10,
        .height = 10,
        .delay = 200,
        .timeout = 10,
        .seed = 67,
        .view = "../vista",             //PARA TESTEO LO DEJAMOS PERO TIENE QUE IR VACIO POR DEFAULT
        .players = { "../jugador" },    //PARA TESTEO LO DEJAMOS PERO TIENE QUE IR VACIO POR DEFAULT
        .amount_players = 1
    };
    return p;
}



/* =========================
   Generic parser functions
   ========================= */

const Parameter *find_parameter(const char *arg, const Parameter params[], int param_count) {
    for (int i = 0; i < param_count; i++) {
        if (strcmp(arg, params[i].flag) == 0) {
            return &params[i];
        }
    }
    return NULL;
}

// void print_help(const char *program_name, const Parameter params[], int param_count) {
//     printf("Usage: %s [options]\n", program_name);
//     printf("Options:\n");

//     for (int i = 0; i < param_count; i++) {
//         printf("  %-5s %s\n", params[i].flag, params[i].help);
//     }
// }

int parse_parameters(int argc, char *argv[], Parameters *paramsConfig) {
    int param_count = sizeof(parameters) / sizeof(parameters[0]);
    int players_flag_seen = 0;
    
    for (int i = 1; i < argc; i++) {
        const Parameter *param = find_parameter(argv[i], parameters, param_count);

        if (param == NULL) {
            fprintf(stderr, "Error: unknown parameter '%s'\n", argv[i]);
            return 0;
        }

        if (strcmp(param->flag, "-p") == 0) {
            int consumed = 0;

            if (!players_flag_seen) {
                paramsConfig->amount_players = 0;
                players_flag_seen = 1;
            }

            while (i + 1 < argc && !is_flag(argv[i + 1])) {
                if (!param->handler(argv[i + 1], paramsConfig)) {
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

        if (param->expects_value) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: parameter '%s' expects a value\n", argv[i]);
                return 0;
            }

            const char *value = argv[i + 1];

            if (!param->handler(value, paramsConfig)) {
                return 0;
            }

            i++; // skip the value
        } else {
            if (!param->handler(NULL, paramsConfig)) {
                return 0;
            }
        }
    }

    if (paramsConfig->amount_players < 1 || paramsConfig->amount_players > 9) {
        fprintf(stderr, "Error: amount of players must be between 1 and 9\n");
        return 0;
    }

    return 1;
}

/* =========================
      Parameter functions
   ========================= */

int handle_width(const char *value, void *context) {
    Parameters *cfg = (Parameters *)context;
    cfg->width = atoi(value);
    return 1;
}

int handle_height(const char *value, void *context) {
    Parameters *cfg = (Parameters *)context;
    cfg->height = atoi(value);
    return 1;
}

int handle_timeout(const char *value, void *context) {
    Parameters *cfg = (Parameters *)context;
    cfg->timeout = atoi(value);
    return 1;
}

int handle_delay(const char *value, void *context) {
    Parameters *cfg = (Parameters *)context;
    cfg->delay = atoi(value);
    return 1;
}

int handle_seed(const char *value, void *context) {
    Parameters *cfg = (Parameters *)context;
    cfg->seed = atoi(value);
    return 1;
}

int handle_view(const char *value, void *context) {
    Parameters *cfg = (Parameters *)context;
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

int handle_players(const char *value, void *context) {
    Parameters *cfg = (Parameters *)context;
    size_t len = strlen(value);
    char *copy;

    if (cfg->amount_players >= 9) {
        fprintf(stderr, "Error: maximum number of players is 9\n");
        return 0;
    }

    copy = malloc(len + 1);
    if (copy == NULL) {
        fprintf(stderr, "Error: not enough memory for player path\n");
        return 0;
    }

    memcpy(copy, value, len + 1);
    cfg->players[cfg->amount_players] = copy;
    cfg->amount_players++;
    return 1;
}

static int is_flag(const char *arg) {
    return arg != NULL && arg[0] == '-' && arg[1] != '\0';
}










// int main(int argc, char *argv[]) {
//     
//     if (argc == 1) {
//         print_help(argv[0], parameters, param_count);
//         return 0;
//     }

// }


// void printhwv()
// {
//     printf("Parsed values:\n");
//     printf("  width   = %d\n", config.width);
//     printf("  height  = %d\n", config.height);
//     printf("  verbose = %d\n", config.verbose);
// }
