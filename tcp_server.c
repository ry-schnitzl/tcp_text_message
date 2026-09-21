//
// Created by rfs4b on 9/16/2026.
//
#include "tcp_server.h"

char* server_error_message(int error_code) {
    switch (error_code) {
        case ERR_SERVER_SOCKET:
            return "Server Failed to Allocate Socket";
        case ERR_SERVER_LISTEN:
            return "Server Unable to Listen on Socket";
        case ERR_SERVER_ACCEPT:
            return "Server Failed to Accept an Incoming Connection";
        case ERR_SERVER_SELECT:
            return "Server Failed to Coordinate between StdIn and Socket";
        case ERR_SERVER_RECV:
            return "Server Failed to Properly Receive Data";
        case ERR_SERVER_INCOMPLETE_SEND:
            return "Server Failed to Fully Send to Client";
        case ERR_SERVER_INPUT:
            return "Server Failed to Process Input";
        case ERR_SERVER_MSG_FORMAT_UNKNOWN:
            return "Server Received Unknown Message Format";
        case ERR_SERVER_FUNCTION_FAILURE:
            return "Server was Unable to Perform Requested Client Function";
        default:
            return ERR_UNKNOWN;
    }
}

int server_process_message_format(char* msg, char* action) {
    if (msg[0] != '{' || msg[2] != '}') return ERR_SERVER_MSG_FORMAT_UNKNOWN;
    *action = msg[1];
    return SERVER_FORMAT_OFFSET;
}

int server_message_function(char action, char* msg, int offset) {
    switch (action) {
        case SERVER_FORMAT_ENCODE:
            int i = offset;
            while (msg[i] != '\0') {
                msg[i]++;
                i++;
            }
            msg[1] = SERVER_FORMAT_RESPONSE;
            return offset;
        case SERVER_FORMAT_DECODE:
            int j = offset;
            while (msg[j] != '\0') {
                msg[j]--;
                j++;
            }
            msg[1] = SERVER_FORMAT_RESPONSE;
            return offset;
        case SERVER_FORMAT_ECHO:
            msg[1] = SERVER_FORMAT_RESPONSE;
            return offset;
        default:
            return ERR_SERVER_FUNCTION_FAILURE;
    }
}

int server_process_message(char* client, char* msg) {
    int client_offset, server_offset;
    char action;
    client_offset = server_process_message_format(msg, &action);
    if (client_offset < 0) {
        msg = "{s}(!) Internal Server Error: Format Unrecognized";
        return client_offset;
    }

    printf("\r[%s] %s\n> ", client, msg + client_offset);
    server_offset = server_message_function(action, msg, client_offset);
    if (server_offset < 0) {
        msg = "{s}(!) Internal Server Error: Function Application Failed";
        return server_offset;
    }
    printf("\r[Server] %s\n> ", msg + server_offset);
    return 0;
}

int server_handle_input() {
    char server_input[MAX_MESSAGE_LEN];
    if (fgets(server_input, MAX_MESSAGE_LEN, stdin) == NULL) return ERR_SERVER_INPUT;
    server_input[strcspn(server_input, "\n")] = '\0';
    return server_process_input(server_input);
}

int server_process_input(char* input) {
    if (strcmp(input, SIG_SERVER_TERMINATE_MSG) == 0) {
        printf("- - - - - - - - - - - - - - - - - - - - - - - - -\n");
        return SIG_SERVER_TERMINATE;
    }
    if (input[0] == SIG_SERVER_MESSAGE) {
        if (input[1] == ' ') printf("[Server] %s\n", input + 2);
        else printf("[Server] %s\n", input + 1);
    }
    printf("> ");
    return 0;
}

int server_handle_request(int server_fd, struct sockaddr_storage* client_addr) {
    int client_fd;
    int request_status, function_status;
    socklen_t client_size = sizeof *client_addr;

    // Now accept any incoming connections:
    client_fd = accept(server_fd, (struct sockaddr *) client_addr, &client_size);
    if (client_fd < 0) return ERR_SERVER_ACCEPT;

    // Recieve data from the connection
    char msg[MAX_MESSAGE_LEN];
    memset(msg, '\0', MAX_MESSAGE_LEN);
    request_status = recv(client_fd, msg, MAX_MESSAGE_LEN, 0);
    if (request_status == -1) return ERR_SERVER_RECV;

    // Process the data
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr, client_ip, INET_ADDRSTRLEN);
    function_status = server_process_message(client_ip, msg);

    int msg_len = strlen(msg) + 1;
    // Finally send it back
    request_status = send(client_fd, msg, msg_len, 0);
    if (request_status < msg_len) return ERR_SERVER_INCOMPLETE_SEND;

    // Reset things server-side
    close(client_fd);
    return function_status;
}

int server_routine(tcp_server* server) {
    int routine_status;
    int server_fd;

    // Create and bind the socket
    char server_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server->home_ip, server_ip, INET_ADDRSTRLEN);
    printf("Initiating server @ %s:%hu\n\n", server_ip, server->port);

    server_fd = server_create_socket(server);
    if (server_fd == -1) return ERR_SERVER_SOCKET;

    // Listen for connections
    routine_status = listen(server_fd, BACKLOG);
    if (routine_status == -1) return ERR_SERVER_LISTEN;

    // Handle connections as long as no errors or terminations are sent
    printf("Listening for connections...\n");
    printf("- - - - - - - - - - - - - - - - - - - - - - - - -\n");
    printf("> ");

    struct sockaddr_storage client_addr;
    fd_set readfds;

    // We are going to switch between listening for sockets and listening
    // server input
    // Using a technique learned on the internet
    while (routine_status == 0) {
        fflush(stdout);
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(server_fd, &readfds);

        int selection_status = select(server_fd + 1, &readfds, NULL, NULL, NULL);
        if (selection_status < 0) return ERR_SERVER_SELECT;

        if (FD_ISSET(0, &readfds)) routine_status = server_handle_input();
        if (FD_ISSET(server_fd, &readfds)) {
            routine_status = server_handle_request(server_fd, &client_addr);
            if (routine_status == ERR_SERVER_MSG_FORMAT_UNKNOWN ||
                routine_status == ERR_SERVER_FUNCTION_FAILURE) {
                printf("\r(!) Internal Server Error: %s\n> ", server_error_message(routine_status));
                routine_status = 0;
            }
        }
    }

    close(server_fd);
    return routine_status;
}

int server_create_socket(tcp_server* server) {
    // create the socket
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    // pack the socket data
    struct sockaddr_in socket_address;
    socket_address.sin_addr = server->home_ip;
    socket_address.sin_port = ntohs(server->port);
    socket_address.sin_family = AF_INET;
    memset(socket_address.sin_zero, 0, 8);

    // bind the socket to a port
    int result = bind(socket_fd, (struct sockaddr *) &socket_address, sizeof(struct sockaddr_in));
    if (result == -1) return -1;
    return socket_fd;
}
