#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "paramsHandler.h"
#include "structures.h"
/*
Lista de params necesarios, el orden puede ser arbritrario.

[-w width]: Ancho del tablero. Default y mínimo: 10
[-h height]: Alto del tablero. Default y mínimo: 10
[-d delay]: milisegundos que espera el máster cada vez que se imprime el estado. Default: 200
[-t timeout]: Timeout en segundos para recibir solicitudes de movimientos válidos. Default: 10
[-s seed]: Semilla utilizada para la generación del tablero. Default: time(NULL)
[-v view]: Ruta del binario de la vista. Default: Sin vista.
-p player1 player2: Ruta/s de los binarios de los jugadores. Mínimo: 1, Máximo: 9.
*/

int handle_width(const char *value, void *context);
int handle_height(const char *value, void *context);
int handle_view(const char *value, void *context);

Parameter parameters[] = {
    {"-w", 1, handle_width,   "Set width  (example: -w 20)"},
    {"-h", 1, handle_height,  "Set height (example: -h 10)"}
};

Parameters default_parameters(){
    Parameters p = {
        10,            // width
        10,            // height
        200,           // delay
        10,            // timeout
        67,            // seed
        "../vista.c",  // view
        "../jugador.c",// player
        2              // player count
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
    
    for (int i = 1; i < argc; i++) {
        const Parameter *param = find_parameter(argv[i], parameters, param_count);

        if (param == NULL) {
            fprintf(stderr, "Error: unknown parameter '%s'\n", argv[i]);
            return 0;
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