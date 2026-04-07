#include <shmCommon.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int shmCreateFd(const char *name, size_t size, mode_t mode) {
    int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, mode);
    if (fd == -1) {
        return -1;
    }

    if (ftruncate(fd, (off_t)size) == -1) {
        close(fd);
        shm_unlink(name);
        return -1;
    }

    return fd;
}

int shmOpenFd(const char *name, int flags) {
    return shm_open(name, flags, 0);
}

void *shmMapFd(int fd, size_t size, int prot) {
    return mmap(NULL, size, prot, MAP_SHARED, fd, 0);
}

int shmUnmapFd(void *addr, size_t size) {
    return munmap(addr, size);
}

int shmCloseFd(int fd) {
    return close(fd);
}

int shmRemoveFd(const char *name) {
    return shm_unlink(name);
}

void *shmCreateAndMap(const char *name, size_t size, mode_t mode, int *fdOut, int prot) {
    int fd = shmCreateFd(name, size, mode);
    if (fd == -1) {
        return MAP_FAILED;
    }

    void *addr = shmMapFd(fd, size, prot);
    if (addr == MAP_FAILED) {
        close(fd);
        shm_unlink(name);
        return MAP_FAILED;
    }

    if (fdOut != NULL) {
        *fdOut = fd;
    } else {
        close(fd);
    }

    return addr;
}

void *shmOpenAndMap(const char *name, size_t size, int flags, int *fdOut, int prot) {
    int fd = shmOpenFd(name, flags);
    if (fd == -1) {
        return MAP_FAILED;
    }

    void *addr = shmMapFd(fd, size, prot);
    if (addr == MAP_FAILED) {
        close(fd);
        return MAP_FAILED;
    }

    if (fdOut) {
        *fdOut = fd;
    } else {
        close(fd);
    }

    return addr;
}
