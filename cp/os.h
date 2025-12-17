#pragma once
#define OS_H

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#endif


#define MAX_GAMES 10
#define MAX_PLAYERS 4
#define MAX_MOVES 100
#define MAX_GAME_NAME 50
#define MAX_PLAYER_NAME 50
#define MAX_REQUESTS 20
#define SECRET_LENGTH 4

#define SHM_SERVER "/bulls_cows_server"
#define SHM_REQUESTS "/bulls_cows_requests"
#define MUTEX_SERVER "/bulls_cows_mutex"
#define SEM_REQUEST "/bulls_cows_sem_req"
#define SEM_RESPONSE "/bulls_cows_sem_resp"

#define REQ_CREATE 1
#define REQ_JOIN 2
#define REQ_FIND 3
#define REQ_MAKE_MOVE 4
#define REQ_GET_STATE 5

typedef struct {
    int id;
    char name[MAX_PLAYER_NAME];
    int connected;
    int score;
    time_t last_activity;
} Player;

typedef struct {
    int player_id;
    char guess[SECRET_LENGTH + 1];
    int bulls;
    int cows;
    time_t timestamp;
} GameMove;

typedef struct {
    char game_name[MAX_GAME_NAME];
    char secret_number[SECRET_LENGTH + 1];
    int player_count;
    int max_players;
    int current_player_turn;
    Player players[MAX_PLAYERS];
    GameMove moves[MAX_MOVES];
    int move_count;
    int is_active;
    int is_finished;
    char winner[MAX_PLAYER_NAME];
    time_t created_time;
} GameState;

//структура сервера
typedef struct {
    int server_pid;
    int game_count;
    GameState games[MAX_GAMES];
    int server_running;
} GameServer;

//структура запроса от клиента
typedef struct {
    int type;           //тип запроса
    int client_pid;     //PID клиента для ответа
    int request_id;     //ID запроса
    
    union {
        struct {
            char game_name[MAX_GAME_NAME];
            char player_name[MAX_PLAYER_NAME];
            int max_players;
        } create;
        
        struct {
            char game_name[MAX_GAME_NAME];
            char player_name[MAX_PLAYER_NAME];
        } join;
        
        struct {
            char player_name[MAX_PLAYER_NAME];
        } find;
        
        struct {
            int game_index;
            int player_id;
            char guess[SECRET_LENGTH + 1];
        } move;
        
        struct {
            int game_index;
            int player_id;
        } state;
    } data;
    
    time_t timestamp;
} ClientRequest;

//структура ответа сервера
typedef struct {
    int request_id;     //ID оригинального запроса
    int client_pid;     //PID клиента
    int success;        //1 - успех, 0 - ошибка
    
    union {
        struct {
            int game_index;
            int player_id;
        } game_info;
        
        struct {
            int bulls;
            int cows;
        } move_result;
        
        struct {
            char game_name[MAX_GAME_NAME];
            int player_count;
            int max_players;
            int is_active;
            int is_finished;
            int current_player_turn;
            char current_player[MAX_PLAYER_NAME];
            GameMove last_moves[5];
            int last_moves_count;
        } game_state;
    } data;
    
    char error_msg[100];
} ServerResponse;

//для общей памяти
typedef struct {
    ClientRequest requests[MAX_REQUESTS];
    ServerResponse responses[MAX_REQUESTS];
    int request_count;
    int response_count;
} CommunicationChannel;

//shared memory
int create_shm(const char* name, size_t size);
int open_shm(const char* name);
void* map_shm(int fd, size_t size);
int unmap_shm(void* addr, size_t size);
int close_shm(int fd);
int unlink_shm(const char* name);

//мьютекс
int create_mutex(void);
int open_mutex(void);
int lock_mutex(void);
int unlock_mutex(void);
int close_mutex(void);

//семафор
int create_semaphore(const char* name, int initial);
int open_semaphore(const char* name);
int wait_semaphore(const char* name);
int post_semaphore(const char* name);
int close_semaphore(const char* name);
int unlink_semaphore(const char* name);

void sleep_ms(int ms);
time_t get_time(void);
int get_pid(void);
