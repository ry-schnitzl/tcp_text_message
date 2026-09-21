#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include "tcp_client.h"
#include "tcp_server.h"

#define NUM_ARGS 3

#define PORT_DEFAULT_SERVER 8080

#define ERR_ARGS 1
#define ERR_PROG_TYPE 2
#define ERR_PORT 3
#define ERR_IP_ADDR 4

// program will be run will these args:
// $1 -> program type
// $2 -> client ip
#define ARG_PROG_TYPE 1
#define ARG_IP 2

#define PROG_TYPE_SERVER "server"
#define PROG_TYPE_CLIENT "client"


typedef struct args_struct {
    char* type;
    struct in_addr ip_addr;
    int port;
    int err;
} args;

args check_arguments(int argc, char** argv) {
    args program = {NULL, {}, {}, 0};
    // Check that we have the correct number of arguments
    if (argc < NUM_ARGS) {
        printf("Missing required arguments\n");
        printf("Usage: tcp_text_message <type> <ip:port> <message>\n");
        program.err = ERR_ARGS;
        return program;
    } else if (argc > NUM_ARGS) {
        printf("Too many arguments\n");
        printf("Usage: tcp_text_message <type> <ip:port> <message>\n");
        program.err = ERR_ARGS;
        return program;
    }

    // the type must either be 'server' or 'client'
    if (!(strcmp(argv[ARG_PROG_TYPE], PROG_TYPE_SERVER) == 0 ||
          strcmp(argv[ARG_PROG_TYPE], PROG_TYPE_CLIENT) == 0) ) {
        printf("Incorrect program type\n");
        printf("Program can either be run as 'server' or 'client'\n");
        program.err = ERR_PROG_TYPE;
        return program;
    }
    program.type = argv[ARG_PROG_TYPE];

    // Parse the port first since inet_pton does not deal with ports
    // To avoid complicated code, if the port is valid but there is garbage afterwards
    // Ignore the garbage
    char* port = strchr(argv[ARG_IP], ':');
    if (port == NULL) {
        program.port = PORT_DEFAULT_SERVER;
    } else {
        *port = '\0';
        if (sscanf(port + 1, "%d", &program.port) != 1) {
            program.err = ERR_PORT;
            return program;
        }

        // Check if its a valid port as well
        if (program.port < 1024 || program.port > 65535) {
            program.err = ERR_PORT;
            return program;
        }
    }

    // Ip strings must be valid ips
    if (inet_pton(AF_INET, argv[ARG_IP], &program.ip_addr) != 1) {
        program.err = ERR_IP_ADDR;
        return program;
    }
    return program;
}

char* termination_message(int term_code) {
    switch (term_code) {
        case ERR_ARGS:
            return "Program Arguments Unable to be Parsed";
        case ERR_PROG_TYPE:
            return "Invalid Program Type; Valid Program Types: 'server', 'client'";
        case ERR_PORT:
            return "Invalid Port Detected; Ports must be between 1024 and 65535";
        case ERR_IP_ADDR:
            return "Invalid IP Address; Unable to be Parsed";

        case SIG_CLIENT_TERMINATE:
            return "Client has Exited";
        case SIG_SERVER_TERMINATE:
            return "Server has Received Signal to Shutdown";
        default:
            char* other_err_msg = client_error_message(term_code);
            if (strcmp(other_err_msg, ERR_UNKNOWN) == 0)
                other_err_msg = server_error_message(term_code);
            if (strcmp(other_err_msg, ERR_UNKNOWN) == 0)
                other_err_msg = "Unknown Error Message";
            return other_err_msg;
    }
}

int print_termination_message(int term_code) {
    char* msg = termination_message(term_code);
    printf("(!) Program Terminated: %s\n", msg);
    if (term_code < 0) printf(" -> Error Code: %d\n", errno);
    return term_code;
}

int main(int argc, char **argv) {
    args program = check_arguments(argc, argv);

    if (program.err != 0) return print_termination_message(program.err);

    int status = ERR_PROG_TYPE;
    if (strcmp(program.type, PROG_TYPE_SERVER) == 0) {
        tcp_server server = {program.ip_addr, (unsigned short int) program.port};
        status = server_routine(&server);
    } else if (strcmp(program.type, PROG_TYPE_CLIENT) == 0) {
        tcp_client client = {
                {AF_INET,
                 htons((unsigned short int) program.port),
                 program.ip_addr},
                '#'};
        memset(client.socket_info.sin_zero, 0, sizeof(client.socket_info.sin_zero));
        status = client_routine(&client);
    }
    return print_termination_message(status);
}
