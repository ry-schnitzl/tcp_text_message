//
// Created by rfs4b on 9/16/2026.
//

#ifndef CS_576_TCP_SERVER_H
#define CS_576_TCP_SERVER_H

#include <netinet/in.h>
#include <malloc.h>
#include <memory.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BACKLOG 10
#define MAX_MESSAGE_LEN 256

#define ERR_SERVER_SOCKET (-10)
#define ERR_SERVER_LISTEN (-11)
#define ERR_SERVER_ACCEPT (-12)
#define ERR_SERVER_RECV (-13)
#define ERR_SERVER_SELECT (-14)
#define ERR_SERVER_INCOMPLETE_SEND (-15)
#define ERR_SERVER_INPUT (-16)
#define ERR_SERVER_MSG_FORMAT_UNKNOWN (-17)
#define ERR_SERVER_FUNCTION_FAILURE (-18)
#define ERR_RESPONSE_UNAUTHORIZED (-30)
#define ERR_UNKNOWN "(?) Unknown Error"

#define SIG_SERVER_MESSAGE '#'
#define SIG_SERVER_TERMINATE_MSG "!off"
#define SIG_SERVER_TERMINATE 10

#define SERVER_FORMAT_ENCODE 'e'
#define SERVER_FORMAT_DECODE 'd'
#define SERVER_FORMAT_ECHO '#'
#define SERVER_FORMAT_RESPONSE 's'

#define SERVER_FORMAT_OFFSET 3

typedef struct tcp_server_struct {
    struct in_addr home_ip;
    unsigned short int port;

} tcp_server;

char* server_error_message(int error_code);

int server_handle_input();

int server_process_input(char* input);

int server_process_message_format(char* msg, char* action);

int server_message_function(char action, char* msg, int offset);

int server_process_message(char* client, char* msg);

int server_handle_request(int server_fd, struct sockaddr_storage* client_addr);

int server_routine(tcp_server* server);

int server_create_socket(tcp_server* server);

#endif //CS_576_TCP_SERVER_H
