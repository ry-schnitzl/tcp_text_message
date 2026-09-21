//
// Created by rfs4b on 9/16/2026.
//

#ifndef CS_576_TCP_CLIENT_H
#define CS_576_TCP_CLIENT_H

#include <netinet/in.h>

#define ERR_CLIENT_CONNECT (-20)
#define ERR_CLIENT_SEND (-21)
#define ERR_CLIENT_INCOMPLETE_SEND (-22)
#define ERR_CLIENT_RECV (-23)
#define ERR_CLIENT_CONNECT_CLOSED (-24)

#define SIG_CLIENT_TERMINATE_MSG "!q"
#define SIG_CLIENT_TERMINATE 20
#define SIG_CLIENT_ENCODE_MSG "!e"
#define SIG_CLIENT_DECODE_MSG "!d"
#define SIG_CLIENT_ECHO_MSG "!#"
#define SIG_CLIENT_MODE_CHANGE 21

typedef struct tcp_client_struct {
    struct sockaddr_in socket_info;
    char client_mode;
} tcp_client;

char* client_error_message(int error_code);

int client_handle_input(tcp_client* client, char* input);

int client_process_server_response(char* response);

int client_routine(tcp_client* client);

int client_send_message(int socket_fd, char* msg);

int client_create_connection(tcp_client* client);

#endif //CS_576_TCP_CLIENT_H
