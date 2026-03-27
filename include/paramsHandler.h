#ifndef PARAMS_HANDLER_H
#define PARAMS_HANDLER_H

typedef struct {
    const char *flag;
    int expects_value;
    ParamHandler handler;
    const char *help;
} Parameter;

Parameters default_parameters();
int parse_parameters(int argc, char *argv[], Parameters *paramsConfig);
typedef int (*ParamHandler)(const char *value, void *context);


#endif