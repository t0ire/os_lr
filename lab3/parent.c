#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>

#include "os.h"

#define BUFFERSIZE 256
#define SHARED_MEM_SIZE 4096

typedef struct {
    char data[BUFFERSIZE];
    int ready; // 0-нет, 1-да, 2-конец
} shared_data_t;

int main() {
    pid_t pid;
    char filename[BUFFERSIZE];
    
    int shm_fd = shm_open("/numbers_shm", O_CREAT | O_RDWR, 0666);//кусок памяти где будут процессы общаться
    if(shm_fd == -1) {
        perror("shm_open error");
        exit(1);
    }
    
    if(ftruncate(shm_fd, SHARED_MEM_SIZE) == -1) { //размер куска
        perror("ftruncate error");
        exit(1);
    }
    
    //адрес размер права доступа флаги фд смещение(с каккого байта начинать отобр)
    shared_data_t* shared_data = mmap(NULL, SHARED_MEM_SIZE, 
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);//отображаем этот кусок в вируальную память 
    if(shared_data == MAP_FAILED) {
        perror("mmap error");
        exit(1);
    }
    
    memset(shared_data, 0, SHARED_MEM_SIZE);//заполняет обл памяти указ знач (ук на обл знач и размер)
    shared_data->ready = 0;

    printf("ведите имя файла: ");
    if(!fgets(filename, BUFFERSIZE, stdin)) {
        perror("fgets error");
        exit(1);
    }

    filename[strcspn(filename, "\n")] = 0;

    pid = CloneProcess();
    if(pid == -1) {
        perror("fork error");
        exit(1);
    }

    if(pid == 0) {
        if(Exec("./child", (char*[]){"child", filename, "/numbers_shm", NULL}) == -1) {
            perror("exec error");
            exit(1);
        }
    } else {
        printf("введите числа, для завершения введите 'stop'\n");
        
        while(1) {
            char buffer[BUFFERSIZE];
            printf("\n");
            fflush(stdout);
            
            if(!fgets(buffer, BUFFERSIZE, stdin)) {
                perror("fgets error");
                break;
            }
            
            buffer[strcspn(buffer, "\n")] = 0;
            
            if(strcmp(buffer, "stop") == 0) {
                strcpy(shared_data->data, "stop");
                shared_data->ready = 2; //конец
                break;
            }
            
            strncpy(shared_data->data, buffer, BUFFERSIZE - 1); //передаем в кусок
            shared_data->data[BUFFERSIZE - 1] = '\0';
            shared_data->ready = 1; //дочь может работать
            
            if(shared_data->ready == 2) {
                break;
            }
        }
    
        WaitProcess();
        
        munmap(shared_data, SHARED_MEM_SIZE); //закрываем кусок
        close(shm_fd); //закрывает фд куска
        shm_unlink("/numbers_shm"); //удаляет кусок 
        
    }
    
    return 0;
}