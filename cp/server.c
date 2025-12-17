#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "game_logic.h"

static volatile int running = 1;

void handle_signal(int sig) {
    running = 0;
    printf("\nсервер: получен сигнал %d, завершение работы\n", sig);
}

int main() {
    printf("=== сервер ===\n");
    
    //обработка сигналов
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    //создаем shared memory для сервера
    int shm_server_fd = create_shm(SHM_SERVER, sizeof(GameServer));
    if (shm_server_fd == -1) {
        fprintf(stderr, "Failed to create server shared memory\n");
        return 1;
    }
    
    //создаем shared memory для запросов
    int shm_requests_fd = create_shm(SHM_REQUESTS, sizeof(CommunicationChannel));
    if (shm_requests_fd == -1) {
        fprintf(stderr, "Failed to create requests shared memory\n");
        close_shm(shm_server_fd);
        unlink_shm(SHM_SERVER);
        return 1;
    }
    
    GameServer* server = map_shm(shm_server_fd, sizeof(GameServer));
    CommunicationChannel* channel = map_shm(shm_requests_fd, sizeof(CommunicationChannel));
    
    if (!server || !channel) {
        fprintf(stderr, "Failed to map shared memory\n");
        if (server) unmap_shm(server, sizeof(GameServer));
        if (channel) unmap_shm(channel, sizeof(CommunicationChannel));
        close_shm(shm_server_fd);
        close_shm(shm_requests_fd);
        unlink_shm(SHM_SERVER);
        unlink_shm(SHM_REQUESTS);
        return 1;
    }
    
    //инициализируем канал связи
    memset(channel, 0, sizeof(CommunicationChannel));
    
    if (create_mutex() == -1) {
        fprintf(stderr, "Failed to create mutex\n");
        unmap_shm(server, sizeof(GameServer));
        unmap_shm(channel, sizeof(CommunicationChannel));
        close_shm(shm_server_fd);
        close_shm(shm_requests_fd);
        unlink_shm(SHM_SERVER);
        unlink_shm(SHM_REQUESTS);
        return 1;
    }
    
    if (create_semaphore(SEM_REQUEST, 0) == -1 ||
        create_semaphore(SEM_RESPONSE, 0) == -1) {
        fprintf(stderr, "Failed to create semaphores\n");
        close_mutex();
        unmap_shm(server, sizeof(GameServer));
        unmap_shm(channel, sizeof(CommunicationChannel));
        close_shm(shm_server_fd);
        close_shm(shm_requests_fd);
        unlink_shm(SHM_SERVER);
        unlink_shm(SHM_REQUESTS);
        return 1;
    }
    
    //инициализируем сервер
    lock_mutex();
    init_server(server);
    unlock_mutex();
    
    printf("сервер запущен (PID: %d)\n", get_pid());
    printf("создана разделяемая память:\n");
    printf(" - данные сервера: %s\n", SHM_SERVER);
    printf(" - запросы: %s\n", SHM_REQUESTS);
    printf(" - мьютекс: %s\n", MUTEX_SERVER);
    printf(" - семафоры: %s, %s\n", SEM_REQUEST, SEM_RESPONSE);
    printf("\nнажмите Ctrl+C, чтобы остановить сервер\n");
    
    while (running && server->server_running) {
        if (wait_semaphore(SEM_REQUEST) == 0) {
            lock_mutex();
            process_client_requests(server, channel);
            
            if (channel->response_count > 0) {
                post_semaphore(SEM_RESPONSE);
            }
            
            unlock_mutex();
        }
        
        lock_mutex();
        
        activate_games(server);
        
        check_inactive_players(server);
        
        remove_old_games(server);
        
        unlock_mutex();
        
        sleep_ms(50);
    }
    
    printf("\nсервер: завершение работы\n");
    
    lock_mutex();
    server->server_running = 0;
    unlock_mutex();
    
    close_mutex();
    unlink_semaphore(SEM_REQUEST);
    unlink_semaphore(SEM_RESPONSE);
    
    unmap_shm(server, sizeof(GameServer));
    unmap_shm(channel, sizeof(CommunicationChannel));
    
    close_shm(shm_server_fd);
    close_shm(shm_requests_fd);
    
    unlink_shm(SHM_SERVER);
    unlink_shm(SHM_REQUESTS);
    
    printf("стоп\n");
    
    return 0;
}