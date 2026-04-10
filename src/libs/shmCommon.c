#include <shmCommon.h>

int shmCreateFd(const char *name, size_t size, mode_t mode){
	/* Crea la memoria compartida. */
    int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, mode);

    if(fd == -1){
		/* Error al crear la memoria compartida. */
        fprintf(stderr, "Error: no se pudo crear la memoria compartida '%s'.\n", name);
        return -1;
    }

    if(ftruncate(fd, (off_t)size) == -1){
		/* Error al establecer el tamaño de la memoria compartida. */
        fprintf(stderr, "Error: no se pudo establecer el tamaño de la memoria compartida '%s'.\n", name);
        close(fd);
        shm_unlink(name);

        return -1;
    }

    return fd;
}

int shmOpenFd(const char *name, int flags){
    return shm_open(name, flags, 0);
}

void *shmMapFd(int fd, size_t size, int prot){
    return mmap(NULL, size, prot, MAP_SHARED, fd, 0);
}

int shmUnmapFd(void *addr, size_t size){
    return munmap(addr, size);
}

int shmCloseFd(int fd){
    return close(fd);
}

int shmRemoveFd(const char *name){
    return shm_unlink(name);
}

void *shmCreateAndMap(const char *name, size_t size, mode_t mode, int *fdOut, int prot){

	/* Crea y mapea la memoria compartida. */
    int fd = shmCreateFd(name, size, mode);

    if(fd == -1){
		/* Error al crear la memoria compartida. */
        fprintf(stderr, "Error: no se pudo crear la memoria compartida '%s'.\n", name);
        return MAP_FAILED;
    }

	/* Mapea la memoria compartida. */
    void *addr = shmMapFd(fd, size, prot);

    if(addr == MAP_FAILED){
		/* Error al mapear la memoria compartida. */
        fprintf(stderr, "Error: no se pudo mapear la memoria compartida '%s'.\n", name);
        close(fd);
        shm_unlink(name);

        return MAP_FAILED;
    }

    if(fdOut != NULL){
        *fdOut = fd;
    }else{
        fprintf(stderr, "Error: no se pudo abrir la memoria compartida '%s'.\n", name);
        close(fd);
    }

    return addr;
}

void *shmOpenAndMap(const char *name, size_t size, int flags, int *fdOut, int prot){

	/* Abre y mapea la memoria compartida. */
    int fd = shmOpenFd(name, flags);

    if(fd == -1){
		/* Error al abrir la memoria compartida. */
        fprintf(stderr, "Error: no se pudo abrir la memoria compartida '%s'.\n", name);
        return MAP_FAILED;
    }

	/* Mapea la memoria compartida. */
    void *addr = shmMapFd(fd, size, prot);

    if(addr == MAP_FAILED){
        /* Error al mapear la memoria compartida. */
        fprintf(stderr, "Error: no se pudo mapear la memoria compartida '%s'.\n", name);
        close(fd);
        return MAP_FAILED;
    }

    if(fdOut){
        *fdOut = fd;
	}else{
        close(fd);
    }

    return addr;
}