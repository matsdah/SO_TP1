#include "structures.h"
#ifndef PARAMS_HANDLER_H
#define PARAMS_HANDLER_H

Parameters default_parameters(void);
int parse_parameters(int argc, char *argv[], Parameters *paramsConfig);

#endif