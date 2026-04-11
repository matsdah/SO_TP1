#ifndef PARAMS_HANDLER_H
#define PARAMS_HANDLER_H

#include <structures.h>

Params defaultParams();

/* Parsea los argumentos de linea de comandos y llena la estructura Params. */
int parseParams(int argc, char *argv[], Params *config);

/* Handler para el flag -p (rutas de jugadores). */
int handlePlayers(const char *value, void *context);

/* Handler para el flag -v (ruta de la vista). */
int handleView(const char *value, void *context);

/* Handler para el flag -s (semilla aleatoria). */
int handleSeed(const char *value, void *context);

/* Handler para el flag -d (delay entre turnos). */
int handleDelay(const char *value, void *context);

/* Handler para el flag -t (timeout de inactividad). */
int handleTimeout(const char *value, void *context);

/* Handler para el flag -h (alto del tablero). */
int handleHeight(const char *value, void *context);

/* Handler para el flag -w (ancho del tablero). */
int handleWidth(const char *value, void *context);

/* Libera memoria dinámica asociada a Params (view y players). */
void freeParams(Params *config);

/* Busca un parametro por su flag en el array de parametros. */
const ParamDef *findParam(const char *arg, const ParamDef params[], int paramCount);

#endif
