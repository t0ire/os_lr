#pragma once

#ifdef _WIN32

#else
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef int pipe_t;
typedef int pid_t;

typedef struct {
  pid_t pid;
} proc_info_t;
#endif

typedef int pipe_t;
typedef int pid_t;

int CreatePipe(pipe_t new_pipe[2]);
pid_t CloneProcess();
int Exec(const char* path, char* argv[]);
int LinkFDtoIN(int fd);
int LinkFDtoOUT(int fd);
int OpenObject(const char* path, int flags, int mod);
int WaitProcess();
int ClosePipe(pipe_t pipe);
int WritePipe(pipe_t pipe, void* buffer, int bytes);
int ReadPipe(pipe_t pipe, void* buffer, int bytes);

int CreateSharedMemory(const char* name, int size);
int OpenSharedMemory(const char* name);
void* MapMemory(int fd, int size);
int UnmapMemory(void* addr, int size);
int CloseSharedMemory(int fd);
int UnlinkSharedMemory(const char* name);