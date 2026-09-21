//
// Created by rfs4b on 9/16/2026.
//
#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "tcp_client.h"
#include "tcp_server.h"

char* client_error_message(int error_code) {
    switch (error_code) {
        case ERR_CLIENT_CONNECT:
            return "Client Failed to Connect to Server";
        case ERR_CLIENT_SEND:
            return "Client Failed to Send to Server";
        case ERR_CLIENT_INCOMPLETE_SEND:
            return "Client Failed to Send all Data to Server";
        case ERR_CLIENT_RECV:
            return "Client Failed to Receive Data from Server";
        case ERR_CLIENT_CONNECT_CLOSED:
            return "Server Forcibly Closed the Connection";
        default:
            return ERR_UNKNOWN;
    }
}

int client_handle_input(tcp_client* client, char* input) {
    if (strcmp(input + SERVER_FORMAT_OFFSET, SIG_CLIENT_TERMINATE_MSG) == 0) {
        input = "Ended Text Stream.";
        return SIG_CLIENT_TERMINATE;
    } else if (strcmp(input + SERVER_FORMAT_OFFSET, SIG_CLIENT_ENCODE_MSG) == 0) {
        client->client_mode = SERVER_FORMAT_ENCODE;
        return SIG_CLIENT_MODE_CHANGE;
    } else if (strcmp(input + SERVER_FORMAT_OFFSET, SIG_CLIENT_DECODE_MSG) == 0) {
        client->client_mode = SERVER_FORMAT_DECODE;
        return SIG_CLIENT_MODE_CHANGE;
    } else if (strcmp(input + SERVER_FORMAT_OFFSET, SIG_CLIENT_ECHO_MSG) == 0) {
        client->client_mode = SERVER_FORMAT_ECHO;
        return SIG_CLIENT_MODE_CHANGE;
    }

    input[0] = '{';
    input[1] = client->client_mode;
    input[2] = '}';
    return 0;
}

int client_process_server_response(char* response) {
    if (response[0] != '{' || response[2] != '}') return ERR_SERVER_MSG_FORMAT_UNKNOWN;
    if (response[1] != SERVER_FORMAT_RESPONSE) return ERR_RESPONSE_UNAUTHORIZED;
    return SERVER_FORMAT_OFFSET;
}

int client_routine(tcp_client* client) {
    char server_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client->socket_info.sin_addr, server_ip, INET_ADDRSTRLEN);
    printf("Initiating client @ %s\n\n", server_ip);

    // Setup the text stream
    printf("Beginning text stream...\n");
    printf("- - - - - - - - - - - - - - - - - - - - - - - - -\n");

    // Send and receive messages to the server
    // Give a prompt, input a message, then send and spit out the result
    char message[MAX_MESSAGE_LEN];
    memset(message, '\0', MAX_MESSAGE_LEN);
    int socket = 0;
    int text_status = 0;
    while (1 == 1) {
        // Detect input
        printf("\r> ");
        fgets(message + SERVER_FORMAT_OFFSET, MAX_MESSAGE_LEN - SERVER_FORMAT_OFFSET, stdin);
        message[SERVER_FORMAT_OFFSET + strcspn(message + SERVER_FORMAT_OFFSET, "\n")] = '\0';

        // Handle client input before its sent so we don't get an error
        // Process any commands, and add formatting
        text_status = client_handle_input(client, message);
        if (text_status == SIG_CLIENT_MODE_CHANGE) {
            printf("(#) Client Mode Changed\n");
            continue;
        }
        if (text_status != 0) {
            printf("- - - - - - - - - - - - - - - - - - - - - - - - -\n");
            return text_status;
        }

        // Create the connection after the input is given so we dont hog server-side
        socket = client_create_connection(client);
        if (socket < 0) {
            if (socket == ERR_CLIENT_CONNECT) {
                printf("(!) Refused connection (Server might be offline)\n");
            } else {
                printf("(!) Error: %s\n", client_error_message(socket));
            }
            continue;
        }

        // Once all checks have been made, send the formatted message
        text_status = client_send_message(socket, message);
        if (text_status < 0) {
            printf("(!) Error: %s\n", client_error_message(socket));
            continue;
        }

        // Double check to make sure we received a valid server response
        // Use the text_status variable as a response offset
        text_status = client_process_server_response(message);
        if (text_status < 0) {
            printf("(!) Error: %s\n", client_error_message(text_status));
            continue;
        }
        printf("[Server] %s\n", message + text_status);
    }

    printf("- - - - - - - - - - - - - - - - - - - - - - - - -\n");
    return text_status;
}

int client_send_message(int socket_fd, char* msg) {
    int msg_status, msg_len;
    msg_len = strlen(msg);
    msg_status = send(socket_fd, msg, msg_len, 0);
    if (msg_status == -1) return ERR_CLIENT_SEND;
    if (msg_status < msg_len) return ERR_CLIENT_INCOMPLETE_SEND;

    msg_status = recv(socket_fd, msg, MAX_MESSAGE_LEN, 0);
    if (msg_status == -1) return ERR_CLIENT_RECV;
    if (msg_status == 0) return ERR_CLIENT_CONNECT_CLOSED;

    close(socket_fd);
    return 0;
}

int client_create_connection(tcp_client* client) {
    // create the socket
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    // then connect to the socket
    int status = connect(socket_fd, (struct sockaddr *) &client->socket_info, sizeof(client->socket_info));
    if (status == -1) return ERR_CLIENT_CONNECT;
    return socket_fd;
}