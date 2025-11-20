#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

int main(int argc, char* argv[]) {
    char buffer[BUFFERSIZE];
    FILE* file;
    
    if(argc < 3) {
        exit(1);
    }

    file = fopen(argv[1], "w");
    if(file == NULL) {
        perror("file open error");
        exit(1);
    }

    int shm_fd = shm_open(argv[2], O_RDWR, 0666); //открываем кусок
    if(shm_fd == -1) {
        perror("shm_open error in child");
        exit(1);
    }
    
    shared_data_t* shared_data = mmap(NULL, SHARED_MEM_SIZE, 
                                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);//отображаем этот кусок в вируальную память 
    if(shared_data == MAP_FAILED) {
        perror("mmap error in child");
        exit(1);
    }
    
    fflush(file);
    
    printf("child запущен\n");
    
    while(1) {
        if(shared_data->ready == 1) {//ждем пока родитель подготовит данные
            strncpy(buffer, shared_data->data, BUFFERSIZE);
            buffer[BUFFERSIZE - 1] = '\0';
            
            if(strcmp(buffer, "stop") == 0) {
                shared_data->ready = 2;
                break;
            }
            
            int valid = 1;
            for(int i = 0; buffer[i] != '\0'; i++) { //проверка на число
                if(!isdigit(buffer[i]) && !isspace(buffer[i]) && 
                   buffer[i] != '-' && buffer[i] != '+') {
                    valid = 0;
                    break;
                }
            }
            
            if(valid && strlen(buffer) > 0) { //сумма

                char* token = strtok(buffer, " ");
                int sum = 0;
                int count = 0;
                char numbers_list[BUFFERSIZE] = "";
                
                while(token != NULL) {
                    int num = atoi(token);
                    sum += num;
                    count++;
                    
                    if(strlen(numbers_list) > 0) {
                        strcat(numbers_list, " ");
                    }
                    strcat(numbers_list, token);
                    
                    token = strtok(NULL, " ");
                }
                
                fprintf(file, "числа: %s   сумма: %d \n", 
                        numbers_list, sum);
                fflush(file);
                
                printf("сумма: %d\n", sum);
            } else {
                fprintf(file, "неверный формат данных: %s\n", buffer);
                fflush(file);
                printf("неверный формат данных: %s\n", buffer);
            }
            
            shared_data->ready = 0;//флаг что все обработали 
        } else if(shared_data->ready == 2) {
            //сигнал завершения от родителя
            break;
        }
        
        //sched_yield(); 
    }
    
    fclose(file);
    
    munmap(shared_data, SHARED_MEM_SIZE);
    close(shm_fd);
    
    printf("child завершен\n");
    return 0;
}