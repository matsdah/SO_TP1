#include "shmCommon.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int createFd(const char * name, size_t size, mode_t mode){
	int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, mode);
	int (fd == -1){
		return -1;
	}

	if(ftruncate(fd, (off_t)size) == -1){
		close(fd);
		shm_unlink(name);
		return -1;
	}

	return fd;
}

int openFd(const char * name, int oFlag){
	return shm_open(name, oFlag, 0);
}

void * mapFd(int fd, size_t size, int prot){
	return mmap(NULL, size, prot, MAP_SHARED, fd, 0);
}

int unmapFd(void * addr, size_t size){
	return munmap(addr, size);
}

int closeFd(int fd){
	return close(fd);
}

int removeFd(const char * name){
	return shm_unlink(name);
}

void * createAndMap(const char * name, size_t size, mode_t mode, int * fdOut, int prot){
	int fd = createFd(name, size, mode);
	if(fd == -1){
		return MAP_FAILED;
	}

	void * addr = mapFd(fd, size, prot);
	if(addr == MAP_FAILED){
		close(fd);
		shm_unlink(name);
		return MAP_FAILED;
	}

	if(fdOut != NULL){
		*fdOut = fd;
	} else{
		close(fd);
	}

	return addr;
}

void * openAndMap(const char * name, size_t size, int oFlag, int * fdOut, int prot);



