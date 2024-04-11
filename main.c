#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_LISTENERS 5
#define SERVER 0
#define CLIENT 1
#define LOCAL 2
#define BUFFER_SIZE 32768

char pathname[50];
int mode = LOCAL;
int port = 12345;
int keep_running = 1;

int builtin_halt(char **args);
int builtin_help(char **args);

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
};


int builtin_halt(char **args) {
    exit(1);
}

int builtin_help(char **args) {
    help();
    return 1;
}

char*** input_parsing(char* input)
{
    return NULL;
}

void client() {
    printf("You are client\n");
    int sock;
    struct sockaddr_in connection_client;
    char output[BUFFER_SIZE], input[BUFFER_SIZE];

    // Create client socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address structure
    connection_client.sin_family = AF_INET;
    connection_client.sin_port = htons(port);
    connection_client.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    if (connect(sock, (struct sockaddr *) &connection_client, sizeof(connection_client)) < 0)
    {
        perror("Socket connection failed");
        exit(EXIT_FAILURE);
    }

    // Handle client communication
    while(1) {
        fgets(input, BUFFER_SIZE, stdin);       // Read input from stdin
        if (strcmp(input, "quit\n") == 0)                   // Check if input contains "quit"
        {
            close(sock);
            break;
        }

        write(sock, input, strlen(input));      // Send input to the server
        read(sock, output, sizeof(output));    // Receive output from the server
        printf("Output: %s", output);                          // Print the received output
    }
}

void server() {
    printf("You are server\n");

    int sock, server_sock;
    struct sockaddr_in connection_server;
    pthread_t client_thread_id, input_thread_id;
    int opt = 1;

    // Create server socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address structure
    connection_server.sin_family = AF_INET;
    connection_server.sin_port = htons(port);
    connection_server.sin_addr.s_addr = INADDR_ANY;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind server socket
    if (bind(sock, (struct sockaddr *) &connection_server, sizeof(connection_server)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sock, MAX_LISTENERS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);


    // Main thread continues to handle incoming connections
    while (1)
    {

    }

    // Close the server socket
    close(sock);
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
    }
    return EXIT_SUCCESS;
}
