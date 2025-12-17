#pragma once
#define GAME_LOGIC_H

#include "os.h"

void generate_secret(char* secret);
int is_valid_guess(const char* guess);
void calculate_bulls_cows(const char* secret, const char* guess, int* bulls, int* cows);

void init_server(GameServer* server);
int server_create_game(GameServer* server, const char* name, const char* player_name, 
                       int max_players, int* game_idx, int* player_id);
int server_join_game(GameServer* server, const char* game_name, 
                     const char* player_name, int* game_idx, int* player_id);
int server_find_game(GameServer* server, const char* player_name, 
                     int* game_idx, int* player_id);
int server_make_move(GameServer* server, int game_idx, int player_id, 
                     const char* guess, int* bulls, int* cows);
void server_get_game_state(GameServer* server, int game_idx, int player_id,
                          ServerResponse* response);

int add_client_request(CommunicationChannel* channel, const ClientRequest* request);
int get_client_response(CommunicationChannel* channel, int client_pid, 
                        int request_id, ServerResponse* response);
void process_client_requests(GameServer* server, CommunicationChannel* channel);

void check_inactive_players(GameServer* server);
void remove_old_games(GameServer* server);
void activate_games(GameServer* server);

int client_send_request(CommunicationChannel* channel, const ClientRequest* request);
int client_wait_response(CommunicationChannel* channel, int client_pid, 
                         int request_id, ServerResponse* response, int timeout_ms);

void print_game_list(GameServer* server);
void print_game_state_from_response(ServerResponse* response);
void print_menu(void);
void print_error(const char* msg);
