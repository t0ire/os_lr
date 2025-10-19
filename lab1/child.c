#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFERSIZE 512

int main(int argc, char *argv[]){ // argv[0] = "child" argv[1] = filename
    if (argc != 2){
        fprintf(stderr, "using: %s filename\n", argv[0]);
        exit(1);
    }

    char buffer[BUFFERSIZE];
    FILE* file;

    file = fopen (argv[1], "w");
    if (file == NULL){
        perror("file trouble");
        exit(1);
    }
    if (fgets(buffer, BUFFERSIZE, stdin) == NULL){
        fprintf(stderr, "not data");
        fclose(file);
        exit(1);
    }

    int sum = 0;
    char* token;
    int namber;

    token = strtok(buffer, " \t\n");

    while (token != NULL){
        if (sscanf(token, "%d", &namber) == 1){
            sum += namber;
        }
        token = strtok(NULL, " \t\n");
    }

    fprintf(file, "%d", sum);
    printf("сумма записана в файл\n");
    fclose(file);

    return 0;
}