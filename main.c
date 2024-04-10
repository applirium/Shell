#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_LISTENERS 5
#define SERVER 0
#define CLIENT 1
#define LOCAL 2
#define BUFFER_SIZE 65536

char pathname[50];
int mode = LOCAL;
int port = 12345;

int builtin_halt(char **args);
int builtin_help(char **args);
int builtin_quit(char **args);

char *builtin_str[] = {
        "halt",
        "help",
        "quit"
};

void help()
{
    printf("Author - Lukáš Štefančík\n"
           "SPAASM Zadanie 2\n"
           "-h for printing help\n"
           "-s for switching into server\n"
           "-c for switching into client\n"
           "-p <port number> to set port\n"
           "\n"
           "Connection modes\n"
           "    0 - SERVER\n"
           "    1 - CLIENT\n"
           "    2 - LOCAL\n"
           "\n"
           "Shell uses these builtins:\n");

    for (int i = 0; i < sizeof(builtin_str) / sizeof(char *); i++) {
        printf("    %3s\n", builtin_str[i]);
    }

    printf("\n"
           "Use man for more information about bash commands\n"
           "\n");
}

int (*builtin_func[]) (char **) = {
        &builtin_halt,
        &builtin_help,
        &builtin_quit
};


int builtin_halt(char **args) {
    exit(1);
}

int builtin_help(char **args) {
    help();
    return 1;
}

int builtin_quit(char **args) {
    return 1;
}

char*** parser(char* input)
{
    return NULL;
}

void client()
{
    printf("You are client\n");
}

void server()
{
    printf("You are server\n");

    int std_in, std_out, sock, server_sock, opt;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in connection_server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    connection_server.sin_family = AF_INET;
    connection_server.sin_port = htons(port);
    connection_server.sin_addr.s_addr = INADDR_ANY;

    std_in = dup(0);
    std_out = dup(1);
    opt = 1;

    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char*) &opt, sizeof(opt)) < 0)
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (bind(sock, (struct sockaddr *) &connection_server, sizeof(connection_server)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, MAX_LISTENERS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);
    server_sock = accept(sock, NULL, NULL);

    if (server_sock < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    close(sock);

    while(1)
    {
        break;
    }

}

void local()
{
    printf("You are local\n");
}

int main(int argc, char **argv) {
    int opt;
    strcpy(pathname, "./sck");

    while ((opt = getopt(argc, argv, ":hscp:p:")) != -1)
    {
        switch(opt) {
            case 'h':
                help();
                break;
            case 's':
                if(mode != CLIENT)
                {
                    mode = SERVER;
                    break;
                }
                else
                {
                    printf("Program doesn't support both server and client at same time\n");
                    return EXIT_FAILURE;
                }
            case 'c':
                if(mode != SERVER)
                {
                    mode = CLIENT;
                    break;
                }
                else
                {
                    printf("Program doesn't support both server and client at same time\n");
                    return EXIT_FAILURE;
                }
            case 'p':
                char *endptr;
                port = (int) strtol(optarg, &endptr, 10);
                break;

            case ':':
                printf("Option -%c requires an argument.\n", optopt);
                return EXIT_FAILURE;

            case '?':
                printf("Unknown option: -%c\n", optopt);
                return EXIT_FAILURE;

            default:
                break;
        }
    }

    switch(mode) {
        case 0:
            server();
            break;
        case 1:
            client();
            break;
        default:
            local();
    }
    return EXIT_SUCCESS;
}
