#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFERSIZE 512

int main() {
    int pipe1[2];
    int pipe2[2];
    pid_t pid;
    char filename[BUFFERSIZE];
    FILE* file;

    printf("напишите имя файла\n");
    if(fgets(filename, BUFFERSIZE, stdin) == NULL) {
        perror("fgets error");
        exit(1);
    }

    filename[strcspn(filename, "\n")] = 0;

    if(pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe error");
        exit(1);
    }
    pid = fork();
    if(pid == -1) {
        perror("fork error");
        exit(1);
    }

    if(pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);
        execl("./child", "child", filename, (char*)NULL); // для main child
        perror("execl error");
        exit(1);
    } else {
        close(pipe1[0]);
        close(pipe2[1]);
        // пишем в пайп1 fflush(stdout); 
        char buffer[BUFFERSIZE];
        printf("введите числа\n");
        if (fgets(buffer, BUFFERSIZE, stdin) != NULL) {
            write(pipe1[1], buffer, strlen(buffer));
        }
        close(pipe1[1]);
        // читаем из пайп2
        int bytes;
        while((bytes = read(pipe2[0], buffer, BUFFERSIZE)) > 0) {
            fwrite(buffer, 1, bytes, stdout);

        }
        if(bytes < 0) {
            perror("read pipe2 error");
            exit(1);
        }
        close(pipe2[0]);
        wait(NULL);
        return 0;
    }fflush(stdout); 
}