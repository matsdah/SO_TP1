#ifndef SHM_H
#define SHM_H

#include <stddef.h>
#include <sys/types.h>

// Crea una SHM nueva
int createFd(const char * name, size_t size, mode_t mode);

//Abre una SHM existente con flags
int openFd(const char * name, int oFlag);

//Mapea un fd de SHM en memoria compartida
void * mapFd(int fd, size_t size, int prot);

//Desmapea una region mapeada
int unmapFd(void * addr, size_t size);

//Cierra un fd
int closeFd(int fd);

//Elimina el objeto de SHM
int removeFd(const char * name);

void * createAndMap(const char * name, size_t size, mode_t mode, int * fdOut, int prot);

void * openAndMap(const char * name, size_t size, int oFlag, int * fdOut, int prot);

#endif