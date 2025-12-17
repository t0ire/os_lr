#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game_logic.h"

void generate_secret(char* secret) {
    char digits[] = "0123456789";
    
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    
    for (int i = 9; i > 0; i--) {
        int j = rand() % (i + 1);
        char temp = digits[i];
        digits[i] = digits[j];
        digits[j] = temp;
    }
    
    strncpy(secret, digits, SECRET_LENGTH);
    secret[SECRET_LENGTH] = '\0';
}

int is_valid_guess(const char* guess) {
    if (strlen(guess) != SECRET_LENGTH) return 0;
    
    for (int i = 0; i < SECRET_LENGTH; i++) {
        if (!isdigit(guess[i])) return 0;
    }
    
    for (int i = 0; i < SECRET_LENGTH; i++) {
        for (int j = i + 1; j < SECRET_LENGTH; j++) {
            if (guess[i] == guess[j]) return 0;
        }
    }
    
    return 1;
}

void calculate_bulls_cows(const char* secret, const char* guess, int* bulls, int* cows) {
    *bulls = *cows = 0;
    
    for (int i = 0; i < SECRET_LENGTH; i++) {
        for (int j = 0; j < SECRET_LENGTH; j++) {
            if (secret[i] == guess[j]) {
                if (i == j) (*bulls)++;
                else (*cows)++;
                break;
            }
        }
    }
}

void init_server(GameServer* server) {
    memset(server, 0, sizeof(GameServer));
    server->server_pid = get_pid();
    server->server_running = 1;
}

int server_create_game(GameServer* server, const char* name, const char* player_name, 
                       int max_players, int* game_idx, int* player_id) {
    if (server->game_count >= MAX_GAMES) {
        return -1; // достигнут лимит игр
    }
    
    // уникальность имени
    for (int i = 0; i < server->game_count; i++) {
        if (strcmp(server->games[i].game_name, name) == 0) {
            return -2; // имя уже используется
        }
    }
    
    // количество игроков
    if (max_players < 2 || max_players > MAX_PLAYERS) {
        return -3; // некорректное количество игроков
    }
    
    // создаем новую игру
    GameState* game = &server->games[server->game_count];
    memset(game, 0, sizeof(GameState));
    
    strncpy(game->game_name, name, MAX_GAME_NAME - 1);
    game->game_name[MAX_GAME_NAME - 1] = '\0';
    game->max_players = max_players;
    game->player_count = 1;
    game->current_player_turn = 0;
    game->created_time = get_time();
    
    // добавляем создателя
    Player* player = &game->players[0];
    player->id = 1;
    strncpy(player->name, player_name, MAX_PLAYER_NAME - 1);
    player->name[MAX_PLAYER_NAME - 1] = '\0';
    player->connected = 1;
    player->last_activity = get_time();
    
    server->game_count++;
    
    *game_idx = server->game_count - 1;
    *player_id = 1;
    
    printf("cервер: игра '%s' создана игроками '%s' (максимум %d игроков), player_id=%d\n", 
           name, player_name, max_players, *player_id);
    return 0;
}

int server_join_game(GameServer* server, const char* game_name, 
                     const char* player_name, int* game_idx, int* player_id) {
    // находим игру
    int found_idx = -1;
    for (int i = 0; i < server->game_count; i++) {
        if (strcmp(server->games[i].game_name, game_name) == 0) {
            found_idx = i;
            break;
        }
    }
    
    if (found_idx == -1) {
        return -1; // игра не найдена
    }
    
    GameState* game = &server->games[found_idx];
    
    if (game->is_finished) {
        return -2; // игра завершена
    }
    
    if (game->player_count >= game->max_players) {
        return -3; // игра полная
    }
    
    // не в игре ли 
    for (int i = 0; i < game->player_count; i++) {
        if (strcmp(game->players[i].name, player_name) == 0) {
            *game_idx = found_idx;
            *player_id = game->players[i].id;
            return 0; // уже в игре, возвращаем данные
        }
    }
    
    // добавляем игрока
    Player* player = &game->players[game->player_count];
    player->id = game->player_count + 1;
    strncpy(player->name, player_name, MAX_PLAYER_NAME - 1);
    player->name[MAX_PLAYER_NAME - 1] = '\0';
    player->connected = 1;
    player->last_activity = get_time();
    
    game->player_count++;
    
    *game_idx = found_idx;
    *player_id = player->id;
    
    printf("cервер: игрок '%s' присоединился к игре '%s' с player_id=%d\n",
           player_name, game_name, *player_id);
    return 0;
}

int server_find_game(GameServer* server, const char* player_name, 
                     int* game_idx, int* player_id) {
    printf("cервер: поиск игрока '%s' в игре\n", player_name);
    
    // Алгоритм поиска "лучшей" игры для присоединения:
    // 1. Приоритет у активных игр с одним свободным местом
    // 2. Затем активные игры с любыми свободными местами
    // 3. Затем неактивные игры, которые скоро начнутся
    
    int best_game = -1;
    int best_score = -1;
    
    for (int i = 0; i < server->game_count; i++) {
        GameState* game = &server->games[i];
        
        // пропускаем завершенные игры
        if (game->is_finished) continue;
        
        // пропускаем полные игры
        if (game->player_count >= game->max_players) continue;
        
        // проверяем, не в игре ли уже этот игрок
        int already_in_game = 0;
        for (int j = 0; j < game->player_count; j++) {
            if (strcmp(game->players[j].name, player_name) == 0) {
                already_in_game = 1;
                *game_idx = i;
                *player_id = game->players[j].id;
                return 0; // уже в игре
            }
        }
        if (already_in_game) continue;
        
        // вычисляем рейтинг игры
        int score = 0;
        
        if (game->is_active) {
            score += 100; // активная игра имеет высокий приоритет
            
            // чем меньше свободных мест, тем лучше
            score += (game->max_players - game->player_count) * 10;
            
            // игры с 1 свободным местом - максимальный приоритет
            if (game->player_count == game->max_players - 1) {
                score += 50;
            }
        } else {
            // неактивная игра
            score += 50;
            
            // чем больше игроков уже есть, тем лучше
            score += game->player_count * 15;
            
            // игры, которые почти заполнены
            if (game->player_count == game->max_players - 1) {
                score += 30;
            }
        }
        
        if (score > best_score) {
            best_score = score;
            best_game = i;
        }
    }
    
    if (best_game != -1) {
        // присоединяем к найденной игре
        int result = server_join_game(server, server->games[best_game].game_name, 
                                     player_name, game_idx, player_id);
        if (result == 0) {
            printf("cервер: найдена игра '%s' для игрока '%s'\n",
                   server->games[best_game].game_name, player_name);
        }
        return result;
    }
    
    return -1; // не найдено подходящей игры
}

int server_make_move(GameServer* server, int game_idx, int player_id, 
                     const char* guess, int* bulls, int* cows) {
    if (game_idx < 0 || game_idx >= server->game_count) {
        return -1; // неверный индекс игры
    }
    
    GameState* game = &server->games[game_idx];
    
    if (game->is_finished) {
        return -2; // игра завершена
    }
    
    if (!game->is_active) {
        return -3; // игра не активна
    }
    
    // отладочная информация
    printf("Server DEBUG: make_move called for game '%s'\n", game->game_name);
    printf("Server DEBUG: player_id=%d, guess='%s'\n", player_id, guess);
    printf("Server DEBUG: current_player_turn=%d, player_count=%d\n", 
           game->current_player_turn, game->player_count);
    
    // проверяем, чей сейчас ход
    if (game->current_player_turn >= game->player_count) {
        printf("Server DEBUG: ERROR: current_player_turn=%d >= player_count=%d\n",
               game->current_player_turn, game->player_count);
        return -4;
    }
    
    int expected_player_id = game->players[game->current_player_turn].id;
    if (expected_player_id != player_id) {
        printf("Server DEBUG: NOT player's turn! Expected player_id=%d (%s), got player_id=%d\n",
               expected_player_id, game->players[game->current_player_turn].name, player_id);
        return -4; // не ваш ход
    }
    
    printf("Server DEBUG: Correct player's turn: %s (id=%d)\n",
           game->players[game->current_player_turn].name, player_id);
    
    // проверяем валидность предположения
    if (!is_valid_guess(guess)) {
        return -5; // Неверный формат
    }
    
    // рассчитываем быков и коров
    calculate_bulls_cows(game->secret_number, guess, bulls, cows);
    
    // сохраняем ход
    if (game->move_count < MAX_MOVES) {
        GameMove* move = &game->moves[game->move_count];
        move->player_id = player_id;
        strncpy(move->guess, guess, SECRET_LENGTH);
        move->guess[SECRET_LENGTH] = '\0';
        move->bulls = *bulls;
        move->cows = *cows;
        move->timestamp = get_time();
        game->move_count++;
    }
    
    // обновляем активность игрока
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].id == player_id) {
            game->players[i].last_activity = get_time();
            break;
        }
    }
    
    // проверяем победу
    if (*bulls == SECRET_LENGTH) {
        game->is_finished = 1;
        
        // находим имя победителя
        for (int i = 0; i < game->player_count; i++) {
            if (game->players[i].id == player_id) {
                strncpy(game->winner, game->players[i].name, MAX_PLAYER_NAME - 1);
                game->winner[MAX_PLAYER_NAME - 1] = '\0';
                printf("сервер: игра '%s' завершена! победитель: %s\n",
                       game->game_name, game->winner);
                break;
            }
        }
    } else {
        // передаем ход следующему игроку
        int old_turn = game->current_player_turn;
        game->current_player_turn = (game->current_player_turn + 1) % game->player_count;
        
        printf("сервер: поворот изменился с '%s' (id=%d, index=%d) на '%s' (id=%d, index=%d)\n",
               game->players[old_turn].name, game->players[old_turn].id, old_turn,
               game->players[game->current_player_turn].name, 
               game->players[game->current_player_turn].id, game->current_player_turn);
    }
    
    return 0;
}

void server_get_game_state(GameServer* server, int game_idx, int player_id,
                          ServerResponse* response) {
    (void)player_id;  // подавляем warning о неиспользуемом параметре
    
    if (game_idx < 0 || game_idx >= server->game_count) {
        response->success = 0;
        strcpy(response->error_msg, "Invalid game index");
        return;
    }
    
    GameState* game = &server->games[game_idx];
    
    response->success = 1;
    strcpy(response->data.game_state.game_name, game->game_name);
    response->data.game_state.player_count = game->player_count;
    response->data.game_state.max_players = game->max_players;
    response->data.game_state.is_active = game->is_active;
    response->data.game_state.is_finished = game->is_finished;
    response->data.game_state.current_player_turn = game->current_player_turn;
    
    // имя текущего игрока
    if (game->current_player_turn < game->player_count) {
        strcpy(response->data.game_state.current_player, 
               game->players[game->current_player_turn].name);
    } else {
        strcpy(response->data.game_state.current_player, "Unknown");
    }
    
    // последние 5 ходов
    int start = game->move_count > 5 ? game->move_count - 5 : 0;
    response->data.game_state.last_moves_count = game->move_count - start;
    
    for (int i = 0; i < response->data.game_state.last_moves_count; i++) {
        response->data.game_state.last_moves[i] = game->moves[start + i];
    }
}

int add_client_request(CommunicationChannel* channel, const ClientRequest* request) {
    if (channel->request_count >= MAX_REQUESTS) {
        return -1; // переполнение
    }
    
    channel->requests[channel->request_count] = *request;
    channel->request_count++;
    
    return 0;
}

int get_client_response(CommunicationChannel* channel, int client_pid, 
                        int request_id, ServerResponse* response) {
    for (int i = 0; i < channel->response_count; i++) {
        if (channel->responses[i].client_pid == client_pid && 
            channel->responses[i].request_id == request_id) {
            *response = channel->responses[i];
            
            // удаляем обработанный ответ (сдвигаем остальные)
            for (int j = i; j < channel->response_count - 1; j++) {
                channel->responses[j] = channel->responses[j + 1];
            }
            channel->response_count--;
            
            return 0;
        }
    }
    
    return -1; // ответ не найден
}

void process_client_requests(GameServer* server, CommunicationChannel* channel) {
    if (channel->request_count == 0) {
        return;
    }
    
    // обрабатываем все ожидающие запросы
    for (int i = 0; i < channel->request_count; i++) {
        ClientRequest* req = &channel->requests[i];
        ServerResponse resp;
        
        memset(&resp, 0, sizeof(ServerResponse));
        resp.request_id = req->request_id;
        resp.client_pid = req->client_pid;
        
        switch (req->type) {
            case REQ_CREATE: {
                int game_idx, player_id;
                int result = server_create_game(server, 
                    req->data.create.game_name,
                    req->data.create.player_name,
                    req->data.create.max_players,
                    &game_idx, &player_id);
                
                if (result == 0) {
                    resp.success = 1;
                    resp.data.game_info.game_index = game_idx;
                    resp.data.game_info.player_id = player_id;
                } else {
                    resp.success = 0;
                    switch (result) {
                        case -1: strcpy(resp.error_msg, "Server is full"); break;
                        case -2: strcpy(resp.error_msg, "Game name already exists"); break;
                        case -3: strcpy(resp.error_msg, "Invalid number of players"); break;
                        default: strcpy(resp.error_msg, "Unknown error"); break;
                    }
                }
                break;
            }
                
            case REQ_JOIN: {
                int game_idx, player_id;
                int result = server_join_game(server, 
                    req->data.join.game_name,
                    req->data.join.player_name,
                    &game_idx, &player_id);
                
                if (result == 0) {
                    resp.success = 1;
                    resp.data.game_info.game_index = game_idx;
                    resp.data.game_info.player_id = player_id;
                } else {
                    resp.success = 0;
                    switch (result) {
                        case -1: strcpy(resp.error_msg, "Game not found"); break;
                        case -2: strcpy(resp.error_msg, "Game is finished"); break;
                        case -3: strcpy(resp.error_msg, "Game is full"); break;
                        default: strcpy(resp.error_msg, "Unknown error"); break;
                    }
                }
                break;
            }
                
            case REQ_FIND: {
                int game_idx, player_id;
                int result = server_find_game(server, 
                    req->data.find.player_name,
                    &game_idx, &player_id);
                
                if (result == 0) {
                    resp.success = 1;
                    resp.data.game_info.game_index = game_idx;
                    resp.data.game_info.player_id = player_id;
                } else {
                    resp.success = 0;
                    strcpy(resp.error_msg, "No available games found");
                }
                break;
            }
                
            case REQ_MAKE_MOVE: {
                int bulls, cows;
                int result = server_make_move(server, 
                    req->data.move.game_index,
                    req->data.move.player_id,
                    req->data.move.guess,
                    &bulls, &cows);
                
                if (result == 0) {
                    resp.success = 1;
                    resp.data.move_result.bulls = bulls;
                    resp.data.move_result.cows = cows;
                } else {
                    resp.success = 0;
                    switch (result) {
                        case -1: strcpy(resp.error_msg, "Invalid game index"); break;
                        case -2: strcpy(resp.error_msg, "Game is finished"); break;
                        case -3: strcpy(resp.error_msg, "Game is not active"); break;
                        case -4: strcpy(resp.error_msg, "Not your turn"); break;
                        case -5: strcpy(resp.error_msg, "Invalid guess format"); break;
                        default: strcpy(resp.error_msg, "Unknown error"); break;
                    }
                }
                break;
            }
                
            case REQ_GET_STATE: {
                server_get_game_state(server, 
                    req->data.state.game_index,
                    req->data.state.player_id,
                    &resp);
                break;
            }
                
            default:
                resp.success = 0;
                strcpy(resp.error_msg, "Unknown request type");
        }
        
        // добавляем ответ
        if (channel->response_count < MAX_REQUESTS) {
            channel->responses[channel->response_count] = resp;
            channel->response_count++;
        }
    }
    
    // очищаем обработанные запросы
    channel->request_count = 0;
}

void check_inactive_players(GameServer* server) {
    time_t now = get_time();
    
    for (int i = 0; i < server->game_count; i++) {
        GameState* game = &server->games[i];
        
        if (!game->is_active || game->is_finished) continue;
        
        for (int j = 0; j < game->player_count; j++) {
            if (game->players[j].connected && 
                (now - game->players[j].last_activity > 30)) {
                game->players[j].connected = 0;
                printf("сервер: игрок '%s' помечен как неактивный\n",
                       game->players[j].name);
            }
        }
    }
}

void remove_old_games(GameServer* server) {
    time_t now = get_time();
    
    for (int i = 0; i < server->game_count; i++) {
        GameState* game = &server->games[i];
        
        if (game->is_finished && (now - game->created_time > 300)) {
            printf("сервер: удаление старой игры '%s'\n", game->game_name);
            
            // сдвигаем игры
            for (int j = i; j < server->game_count - 1; j++) {
                server->games[j] = server->games[j + 1];
            }
            server->game_count--;
            i--;
        }
    }
}

void activate_games(GameServer* server) {
    for (int i = 0; i < server->game_count; i++) {
        GameState* game = &server->games[i];
        
        if (!game->is_active && !game->is_finished && 
            game->player_count == game->max_players) {
            generate_secret(game->secret_number);
            game->is_active = 1;
            printf("сервер: игра '%s' активирована с секретным ключом: %s\n",
                   game->game_name, game->secret_number);
        }
    }
}

int client_send_request(CommunicationChannel* channel, const ClientRequest* request) {
    lock_mutex();
    
    int result = add_client_request(channel, request);
    
    unlock_mutex();
    
    if (result == 0) {
        // уведомляем сервер о новом запросе
        post_semaphore(SEM_REQUEST);
        return 0;
    }
    
    return -1;
}

int client_wait_response(CommunicationChannel* channel, int client_pid, 
                         int request_id, ServerResponse* response, int timeout_ms) {
    int attempts = timeout_ms / 100;
    
    for (int i = 0; i < attempts; i++) {
        lock_mutex();
        
        int result = get_client_response(channel, client_pid, request_id, response);
        
        unlock_mutex();
        
        if (result == 0) {
            return 0; // ответ получен
        }
        
        sleep_ms(100);
    }
    
    return -1; 
}

void print_game_list(GameServer* server) {
    if (server->game_count == 0) {
        printf("игры недоступны.\n");
        return;
    }
    
    printf("\n=== доступные игры (%d) ===\n", server->game_count);
    for (int i = 0; i < server->game_count; i++) {
        GameState* game = &server->games[i];
        printf("%d. '%s' (%d/%d игроки) - ", 
               i + 1, game->game_name, game->player_count, game->max_players);
        
        if (game->is_finished) {
            printf("конец (победил: %s)\n", game->winner);
        } else if (game->is_active) {
            printf("активно (очередь: %s)\n", 
                   game->players[game->current_player_turn].name);
        } else {
            printf("ожидание игроков\n");
        }
    }
}

void print_game_state_from_response(ServerResponse* response) {
    if (!response->success) {
        printf("не удалось получить состояние игры: %s\n", response->error_msg);
        return;
    }
    
    printf("\n=== игра: %s ===\n", response->data.game_state.game_name);
    printf("игрок: %d/%d | статус: ", 
           response->data.game_state.player_count,
           response->data.game_state.max_players);
    
    if (response->data.game_state.is_finished) {
        printf("конец\n");
    } else if (response->data.game_state.is_active) {
        printf("текущий ход: %s\n", 
               response->data.game_state.current_player);
    } else {
        printf("ждем\n");
    }
    
    if (response->data.game_state.last_moves_count > 0) {
        printf("\nпоследний ход:\n");
        for (int i = 0; i < response->data.game_state.last_moves_count; i++) {
            GameMove* move = &response->data.game_state.last_moves[i];
            printf("  игрок %d: %s (быков: %d, коров: %d)\n", 
                   move->player_id, move->guess, move->bulls, move->cows);
        }
    }
}

void print_menu(void) {
    printf("\n=== быки и коровы ===\n");
    printf("1. создать новую игру\n");
    printf("2. присоединиться к игре по имени\n");
    printf("3. найти игру\n");
    printf("4. показать все игры\n");
    printf("5. выход\n");
    printf("выберите: ");
}

void print_error(const char* msg) {
    printf("Error: %s\n", msg);
}