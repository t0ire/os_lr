#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game_logic.h"

static int client_pid = 0;
static int request_counter = 0;

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int get_request_id() {
    return ++request_counter;
}

int main() {
    static int last_shown_turn_index = -1;
    static int consecutive_wait_count = 0;

    client_pid = get_pid();
    
    printf("=== клиент ===\n");
    printf("PID: %d\n", client_pid);
    
    char player_name[MAX_PLAYER_NAME];
    printf("введите имя: ");
    
    if (!fgets(player_name, sizeof(player_name), stdin)) {
        printf("не удалось прочитать имя\n");
        return 1;
    }
    player_name[strcspn(player_name, "\n")] = '\0';
    
    if (strlen(player_name) == 0) {
        printf("пустое\n");
        return 1;
    }
    
    // открываем shared memory сервера
    int shm_server_fd = open_shm(SHM_SERVER);
    if (shm_server_fd == -1) {
        printf("ошибка: сервер не запущен\n");
        return 1;
    }
    
    // открываем shared memory запросов
    int shm_requests_fd = open_shm(SHM_REQUESTS);
    if (shm_requests_fd == -1) {
        printf("ошибка: невозможно подключиться к серверу\n");
        close_shm(shm_server_fd);
        return 1;
    }
    
    GameServer* server = map_shm(shm_server_fd, sizeof(GameServer));
    CommunicationChannel* channel = map_shm(shm_requests_fd, sizeof(CommunicationChannel));
    
    if (!server || !channel) {
        printf("ошибка: не удалось подключиться к памяти\n");
        if (server) unmap_shm(server, sizeof(GameServer));
        if (channel) unmap_shm(channel, sizeof(CommunicationChannel));
        close_shm(shm_server_fd);
        close_shm(shm_requests_fd);
        return 1;
    }
    
    if (open_mutex() == -1) {
        printf("ошибка: не удалось открыть мьютекс\n");
        unmap_shm(server, sizeof(GameServer));
        unmap_shm(channel, sizeof(CommunicationChannel));
        close_shm(shm_server_fd);
        close_shm(shm_requests_fd);
        return 1;
    }
    
    if (open_semaphore(SEM_REQUEST) == -1 || 
        open_semaphore(SEM_RESPONSE) == -1) {
        printf("ошибка: не удалось открыть семафоры\n");
        close_mutex();
        unmap_shm(server, sizeof(GameServer));
        unmap_shm(channel, sizeof(CommunicationChannel));
        close_shm(shm_server_fd);
        close_shm(shm_requests_fd);
        return 1;
    }
    
    printf("подключено к серверу\n");
    
    int in_game = 0;
    int current_game_index = -1;
    int current_player_id = -1;
    char current_game_name[MAX_GAME_NAME] = "";
    
    // переменные для отслеживания изменений состояния
    static int last_move_count = -1;
    static int last_player_turn = -1;
    static int last_is_finished = -1;
    static int last_is_active = -1;
    static int debug_shown = 0;
    
    while (1) {
        if (!in_game) {
            // основное меню
            print_menu();
            
            int choice;
            if (scanf("%d", &choice) != 1) {
                clear_input_buffer();
                printf("некорректный ввод\n");
                continue;
            }
            clear_input_buffer();
            
            switch (choice) {
                case 1: {
                    // создание игры
                    char game_name[MAX_GAME_NAME];
                    int max_players;
                    
                    printf("введите название игры: ");
                    fgets(game_name, sizeof(game_name), stdin);
                    game_name[strcspn(game_name, "\n")] = '\0';
                    
                    if (strlen(game_name) == 0) {
                        printf("название не может быть пустым\n");
                        break;
                    }
                    
                    printf("максимум игроков (2-4): ");
                    if (scanf("%d", &max_players) != 1) {
                        clear_input_buffer();
                        printf("некорректный ввод\n");
                        break;
                    }
                    clear_input_buffer();
                    
                    if (max_players < 2) max_players = 2;
                    if (max_players > 4) max_players = 4;
                    
                    ClientRequest req;
                    memset(&req, 0, sizeof(ClientRequest));
                    req.type = REQ_CREATE;
                    req.client_pid = client_pid;
                    req.request_id = get_request_id();
                    req.timestamp = get_time();
                    
                    strcpy(req.data.create.game_name, game_name);
                    strcpy(req.data.create.player_name, player_name);
                    req.data.create.max_players = max_players;
                    
                    if (client_send_request(channel, &req) == -1) {
                        printf("не удалось отправить запрос\n");
                        break;
                    }
                    
                    ServerResponse resp;
                    if (client_wait_response(channel, client_pid, req.request_id, 
                                            &resp, 5000) == -1) {
                        printf("сервер не отвечает\n");
                        break;
                    }
                    
                    if (resp.success) {
                        current_game_index = resp.data.game_info.game_index;
                        current_player_id = resp.data.game_info.player_id;
                        strcpy(current_game_name, game_name);
                        in_game = 1;
                        
                        // сбрасываем флаги отображения при входе в новую игру
                        last_move_count = -1;
                        last_player_turn = -1;
                        last_is_finished = -1;
                        last_is_active = -1;
                        debug_shown = 0;
                        
                        printf("игра '%s' создана\n", game_name);
                        printf("ваш ID: %d\n", current_player_id);
                        printf("ожидание %d игроков\n", max_players - 1);
                    } else {
                        printf("не удалось создать игру: %s\n", resp.error_msg);
                    }
                    break;
                }
                    
                case 2: {
                    // присоединение к игре по имени
                    lock_mutex();
                    print_game_list(server);
                    unlock_mutex();
                    
                    char game_name[MAX_GAME_NAME];
                    printf("\nвведите название игры: ");
                    fgets(game_name, sizeof(game_name), stdin);
                    game_name[strcspn(game_name, "\n")] = '\0';
                    
                    if (strlen(game_name) == 0) {
                        printf("название не может быть пустым\n");
                        break;
                    }
                    
                    ClientRequest req;
                    memset(&req, 0, sizeof(ClientRequest));
                    req.type = REQ_JOIN;
                    req.client_pid = client_pid;
                    req.request_id = get_request_id();
                    req.timestamp = get_time();
                    
                    strcpy(req.data.join.game_name, game_name);
                    strcpy(req.data.join.player_name, player_name);
                    
                    if (client_send_request(channel, &req) == -1) {
                        printf("не удалось отправить запрос\n");
                        break;
                    }
                    
                    ServerResponse resp;
                    if (client_wait_response(channel, client_pid, req.request_id, 
                                            &resp, 5000) == -1) {
                        printf("сервер не отвечает\n");
                        break;
                    }
                    
                    if (resp.success) {
                        current_game_index = resp.data.game_info.game_index;
                        current_player_id = resp.data.game_info.player_id;
                        
                        // имя игры
                        lock_mutex();
                        if (current_game_index >= 0 && 
                            current_game_index < server->game_count) {
                            strcpy(current_game_name, 
                                   server->games[current_game_index].game_name);
                        }
                        unlock_mutex();
                        
                        in_game = 1;
                        
                        // сбрасываем флаги отображения при входе в новую игру
                        last_move_count = -1;
                        last_player_turn = -1;
                        last_is_finished = -1;
                        last_is_active = -1;
                        debug_shown = 0;
                        
                        printf("вы в игре '%s'\n", current_game_name);
                        printf("ваш ID: %d\n", current_player_id);
                    } else {
                        printf("не удалось присоединиться: %s\n", resp.error_msg);
                    }
                    break;
                }
                    
                case 3: {
                    printf("поиск игры...\n");
                    
                    ClientRequest req;
                    memset(&req, 0, sizeof(ClientRequest));
                    req.type = REQ_FIND;
                    req.client_pid = client_pid;
                    req.request_id = get_request_id();
                    req.timestamp = get_time();
                    
                    strcpy(req.data.find.player_name, player_name);
                    
                    if (client_send_request(channel, &req) == -1) {
                        printf("не удалось отправить запрос\n");
                        break;
                    }
                    
                    ServerResponse resp;
                    if (client_wait_response(channel, client_pid, req.request_id, 
                                            &resp, 5000) == -1) {
                        printf("сервер не отвечает\n");
                        break;
                    }
                    
                    if (resp.success) {
                        current_game_index = resp.data.game_info.game_index;
                        current_player_id = resp.data.game_info.player_id;
                        
                        lock_mutex();
                        if (current_game_index >= 0 && 
                            current_game_index < server->game_count) {
                            strcpy(current_game_name, 
                                   server->games[current_game_index].game_name);
                        }
                        unlock_mutex();
                        
                        in_game = 1;
                        
                        // сбрасываем флаги отображения при входе в новую игру
                        last_move_count = -1;
                        last_player_turn = -1;
                        last_is_finished = -1;
                        last_is_active = -1;
                        debug_shown = 0;
                        
                        printf("найдена игра '%s'\n", current_game_name);
                        printf("ваш ID: %d\n", current_player_id);
                    } else {
                        printf("игра не найдена: %s\n", resp.error_msg);
                        printf("создайте свою игру\n");
                    }
                    break;
                }
                    
                case 4: {
                    // показать все игры
                    lock_mutex();
                    print_game_list(server);
                    unlock_mutex();
                    break;
                }
                    
                case 5: {
                    // выход
                    printf("выход\n");
                    
                    close_mutex();
                    unmap_shm(server, sizeof(GameServer));
                    unmap_shm(channel, sizeof(CommunicationChannel));
                    close_shm(shm_server_fd);
                    close_shm(shm_requests_fd);
                    
                    return 0;
                }
                    
                default:
                    printf("неверный выбор\n");
            }
            
        } else {
            // внутри игры
            ClientRequest state_req;
            memset(&state_req, 0, sizeof(ClientRequest));
            state_req.type = REQ_GET_STATE;
            state_req.client_pid = client_pid;
            state_req.request_id = get_request_id();
            state_req.timestamp = get_time();
            state_req.data.state.game_index = current_game_index;
            state_req.data.state.player_id = current_player_id;
            
            if (client_send_request(channel, &state_req) == -1) {
                printf("ошибка получения состояния\n");
                sleep_ms(2000);
                continue;
            }
            
            ServerResponse state_resp;
            if (client_wait_response(channel, client_pid, state_req.request_id, 
                                    &state_resp, 3000) == -1) {
                printf("тайм-аут\n");
                sleep_ms(2000);
                continue;
            }
            
            if (!state_resp.success) {
                printf("ошибка: %s\n", state_resp.error_msg);
                sleep_ms(2000);
                continue;
            }
            
            // проверяем, изменилось ли состояние
            int state_changed = 0;
            
            if (last_move_count != state_resp.data.game_state.last_moves_count ||
                last_player_turn != state_resp.data.game_state.current_player_turn ||
                last_is_finished != state_resp.data.game_state.is_finished ||
                last_is_active != state_resp.data.game_state.is_active) {
                
                state_changed = 1;
                
                // обновляем сохраненные значения
                last_move_count = state_resp.data.game_state.last_moves_count;
                last_player_turn = state_resp.data.game_state.current_player_turn;
                last_is_finished = state_resp.data.game_state.is_finished;
                last_is_active = state_resp.data.game_state.is_active;
                
                // выводим заголовок игры при изменении
                printf("\n=== игра: %s ===\n", state_resp.data.game_state.game_name);
                printf("игроки: %d/%d | статус: ", 
                       state_resp.data.game_state.player_count,
                       state_resp.data.game_state.max_players);
                
                if (state_resp.data.game_state.is_finished) {
                    printf("завершена\n");
                } else if (state_resp.data.game_state.is_active) {
                    printf("активна | ход: %s\n", 
                           state_resp.data.game_state.current_player);
                } else {
                    printf("ожидание\n");
                }
            }
            
            // проверяем завершение игры
            if (state_resp.data.game_state.is_finished) {
                if (state_changed) {
                    printf("\n=== ИГРА ЗАВЕРШЕНА ===\n");
                    printf("нажмите Enter для выхода в меню");
                    getchar();
                } else {
                    sleep_ms(1000);
                }
                continue;
            }
            
            // проверяем активность игры
            if (!state_resp.data.game_state.is_active) {
                if (state_changed) {
                    printf("ожидание игроков...\n");
                    printf("игроков: %d/%d\n", 
                           state_resp.data.game_state.player_count,
                           state_resp.data.game_state.max_players);
                }
                sleep_ms(3000);
                continue;
            }
            
            // определяем, чей сейчас ход
            int is_my_turn = 0;
            
            // отладочную информацию показываем только при изменении состояния
            if (state_changed || !debug_shown) {
                debug_shown = 1;
                
                // последние ходы показываем только если они есть и изменились
                if (state_resp.data.game_state.last_moves_count > 0) {
                    printf("\nпоследние ходы:\n");
                    for (int i = 0; i < state_resp.data.game_state.last_moves_count; i++) {
                        GameMove* move = &state_resp.data.game_state.last_moves[i];
                        printf("  игрок %d: %s (быки: %d, коровы: %d)\n", 
                               move->player_id, move->guess, move->bulls, move->cows);
                    }
                }
                
                // проверка по имени
                if (strcmp(state_resp.data.game_state.current_player, player_name) == 0) {
                    is_my_turn = 1;
                    printf("\n>>> ВАШ ХОД <<<\n");
                } else {
                    printf("\n>>> ход: %s <<<\n", 
                           state_resp.data.game_state.current_player);
                }
            } else {
                // если состояние не изменилось, просто определяем чей ход
                if (strcmp(state_resp.data.game_state.current_player, player_name) == 0) {
                    is_my_turn = 1;
                }
            }
            
            // сброс отладки при смене состояния
            if (state_changed) {
                debug_shown = 0;
            }
            
            if (is_my_turn) {
                // мой ход
                char guess[10];
                printf("введите %d цифр: ", SECRET_LENGTH);
                fgets(guess, sizeof(guess), stdin);
                guess[strcspn(guess, "\n")] = '\0';
    
                if (!is_valid_guess(guess)) {
                    printf("ошибка! %d уникальных цифр (0-9)\n", SECRET_LENGTH);
                    sleep_ms(1000);
                    continue;
                }
                
                // отправляем ход серверу
                ClientRequest move_req;
                memset(&move_req, 0, sizeof(ClientRequest));
                move_req.type = REQ_MAKE_MOVE;
                move_req.client_pid = client_pid;
                move_req.request_id = get_request_id();
                move_req.timestamp = get_time();
                move_req.data.move.game_index = current_game_index;
                move_req.data.move.player_id = current_player_id;
                strcpy(move_req.data.move.guess, guess);
                
                if (client_send_request(channel, &move_req) == -1) {
                    printf("ошибка отправки\n");
                    sleep_ms(1000);
                    continue;
                }
                
                ServerResponse move_resp;
                if (client_wait_response(channel, client_pid, move_req.request_id, 
                                        &move_resp, 3000) == -1) {
                    printf("тайм-аут\n");
                    sleep_ms(1000);
                    continue;
                }
                
                if (move_resp.success) {
                    printf("ваш ход: %s (быки: %d, коровы: %d)\n",
                        guess, move_resp.data.move_result.bulls, 
                        move_resp.data.move_result.cows);
                    
                    if (move_resp.data.move_result.bulls == SECRET_LENGTH) {
                        printf("\nПОБЕДА! вы угадали!\n");
                        printf("нажмите Enter\n");
                        getchar();
                    }
                } else {
                    printf("ошибка: %s\n", move_resp.error_msg);
                }
                
            } else {
                // ход другого игрока - минимальный вывод
                printf(".");
                fflush(stdout);
                sleep_ms(2500);
            }
        }
    }
    return 0;
}