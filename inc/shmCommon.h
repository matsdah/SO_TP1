#ifndef SHM_COMMON_H
#define SHM_COMMON_H

#include <stddef.h>
#include <sys/types.h>

/* Crea un objeto de memoria compartida con el nombre y tamano especificado. */
int shmCreateFd(const char *name, size_t size, mode_t mode);

/* Abre un objeto de memoria compartida existente. */
int shmOpenFd(const char *name, int flags);

/* Mapea un file descriptor de memoria compartida en el espacio de direcciones. */
void *shmMapFd(int fd, size_t size, int prot);

/* Desmapea una region de memoria compartida. */
int shmUnmapFd(void *addr, size_t size);

/* Cierra un file descriptor de memoria compartida. */
int shmCloseFd(int fd);

/* Elimina un objeto de memoria compartida por nombre. */
int shmRemoveFd(const char *name);

/* Crea y mapea memoria compartida en una sola operacion. */
void *shmCreateAndMap(const char *name, size_t size, mode_t mode, int *fdOut, int prot);

/* Abre y mapea memoria compartida existente en una sola operacion. */
void *shmOpenAndMap(const char *name, size_t size, int flags, int *fdOut, int prot);

#endif
