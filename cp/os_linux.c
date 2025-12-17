// os_linux.c
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>

#include "os.h"

// для синхронизации
static pthread_mutex_t* global_mutex = NULL;
static int shm_fd_mutex = -1;

int create_shm(const char* name, size_t size) {
    shm_unlink(name);
    
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open create");
        return -1;
    }
    
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        close(fd);
        return -1;
    }
    
    return fd;
}

int open_shm(const char* name) {
    int fd = shm_open(name, O_RDWR, 0666);
    if (fd == -1) {
        return -1;
    }
    return fd;
}

void* map_shm(int fd, size_t size) {
    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    return addr;
}

int unmap_shm(void* addr, size_t size) {
    return munmap(addr, size);
}

int close_shm(int fd) {
    return close(fd);
}

int unlink_shm(const char* name) {
    return shm_unlink(name);
}

int create_mutex(void) {
    shm_fd_mutex = shm_open(MUTEX_SERVER, O_CREAT | O_RDWR, 0666);
    if (shm_fd_mutex == -1) {
        perror("shm_open for mutex");
        return -1;
    }
    
    if (ftruncate(shm_fd_mutex, sizeof(pthread_mutex_t)) == -1) {
        perror("ftruncate for mutex");
        close(shm_fd_mutex);
        return -1;
    }

    global_mutex = (pthread_mutex_t*)mmap(NULL, sizeof(pthread_mutex_t), 
                                      PROT_READ | PROT_WRITE, MAP_SHARED, 
                                      shm_fd_mutex, 0);
    if (global_mutex == MAP_FAILED) {
        perror("mmap for mutex");
        close(shm_fd_mutex);
        return -1;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(global_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    return 0;
}

int open_mutex(void) {
    shm_fd_mutex = shm_open(MUTEX_SERVER, O_RDWR, 0666);
    if (shm_fd_mutex == -1) {
        return -1;
    }
    
    global_mutex = (pthread_mutex_t*)mmap(NULL, sizeof(pthread_mutex_t), 
                                      PROT_READ | PROT_WRITE, MAP_SHARED, 
                                      shm_fd_mutex, 0);
    if (global_mutex == MAP_FAILED) {
        close(shm_fd_mutex);
        return -1;
    }
    
    return 0;
}

int lock_mutex(void) {
    if (!global_mutex) return -1;
    return pthread_mutex_lock(global_mutex);
}

int unlock_mutex(void) {
    if (!global_mutex) return -1;
    return pthread_mutex_unlock(global_mutex);
}

int close_mutex(void) {
    if (global_mutex) {
        pthread_mutex_destroy(global_mutex);
        munmap(global_mutex, sizeof(pthread_mutex_t));
    }
    if (shm_fd_mutex != -1) {
        close(shm_fd_mutex);
    }
    return 0;
}

int create_semaphore(const char* name, int initial) {
    sem_unlink(name);
    
    sem_t* sem = sem_open(name, O_CREAT, 0666, initial);
    if (sem == SEM_FAILED) {
        perror("sem_open create");
        return -1;
    }
    sem_close(sem);
    return 0;
}

int open_semaphore(const char* name) {
    sem_t* sem = sem_open(name, O_RDWR);
    if (sem == SEM_FAILED) {
        return -1;
    }
    sem_close(sem);
    return 0;
}

int wait_semaphore(const char* name) {
    sem_t* sem = sem_open(name, O_RDWR);
    if (sem == SEM_FAILED) {
        return -1;
    }
    
    int result = sem_wait(sem);
    sem_close(sem);
    return result;
}

int post_semaphore(const char* name) {
    sem_t* sem = sem_open(name, O_RDWR);
    if (sem == SEM_FAILED) {
        return -1;
    }
    
    int result = sem_post(sem);
    sem_close(sem);
    return result;
}

int close_semaphore(const char* name) {
    (void)name;  // подавляем warning о неиспользуемом параметре
    return 0;
}

int unlink_semaphore(const char* name) {
    return sem_unlink(name);
}

void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

time_t get_time(void) {
    return time(NULL);
}

int get_pid(void) {
    return getpid();
}