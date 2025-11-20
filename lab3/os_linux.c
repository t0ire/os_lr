#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "os.h"

int CreatePipe(pipe_t new_pipe[2]) { return pipe(new_pipe); }
pid_t CloneProcess() { return fork(); }
int Exec(const char* path, char* argv[]) { return execv(path, argv); }
int LinkFDtoIN(int fd) { return dup2(fd, STDIN_FILENO); }
int LinkFDtoOUT(int fd) { return dup2(fd, STDOUT_FILENO); }
int OpenObject(const char* path, int flags, int mod) { return open(path, flags, mod); }
int WaitProcess() { return wait(NULL); }
int ClosePipe(pipe_t pipe) { return close(pipe); }
int WritePipe(pipe_t pipe, void* buffer, int bytes) { return write(pipe, buffer, bytes); }
int ReadPipe(pipe_t pipe, void* buffer, int bytes) { return read(pipe, buffer, bytes); }


int CreateSharedMemory(const char* name, int size) {
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd != -1 && size > 0) {
        ftruncate(fd, size);  
    }
    return fd;
}

int OpenSharedMemory(const char* name) {
    return shm_open(name, O_RDWR, 0666);
}

void* MapMemory(int fd, int size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

int UnmapMemory(void* addr, int size) {
    return munmap(addr, size);
}

int CloseSharedMemory(int fd) {
    return close(fd);
}

int UnlinkSharedMemory(const char* name) {
    return shm_unlink(name);
}