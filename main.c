#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
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

char* execute(char* input)
{
    //TODO execution
    return "doing bullshit\n";
}

void client() {
    printf("You are client\n");

    int sock;
    struct sockaddr_in connection_client;
    char output[BUFFER_SIZE], input[BUFFER_SIZE];

    // Create client socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
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
        memset(input, 0, sizeof(input));
        memset(output, 0, sizeof(output));

        fgets(input, BUFFER_SIZE, stdin);        // Read input from stdin
        write(sock, input, strlen(input));      // Send input to the server
        if (strcmp(input, "quit\n") != 0)                     // Check if input contains "quit"
        {
            read(sock, output, sizeof(output));    // Receive output from the server
            printf("Client: %s", output);                  // Print the received output
        }
        else
        {
            close(sock);
            return;
        }
    }
}

void *handle_server_input(void *arg) {
    int server_sock = *((int *)arg);
    char output[BUFFER_SIZE], input[BUFFER_SIZE];

    while (1) {
        // Read input from stdin
        memset(input, 0, sizeof(input));
        memset(output, 0, sizeof(output));

        fgets(input, BUFFER_SIZE, stdin);

        if (strcmp(input, "quit\n") == 0) {
            // Close the server socket
            close(server_sock);                 //TODO quit from server

            // Set the flag to stop the server loop
            keep_running = 0;
            break;
        }
        else
        {
            strcpy(output, execute(input));
            printf("Server: %s",output);
        }
    }

    return NULL;
}

void *handle_client(void *arg) {
    int client_sock = *((int *)arg);
    char input[BUFFER_SIZE], output[BUFFER_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_sock, input, BUFFER_SIZE, 0)) > 0)
    {
        input[bytes_received] = '\0'; // Null-terminate the received data

        // Check if the client wants to quit
        if (strcmp(input, "quit\n") == 0) {
            printf("Client disconnected.\n");
            break;
        }
        else
        {
            strcpy(output, execute(input));
            write(client_sock, output, strlen(output));      // Send input to the server     // Echo the message back to the client
        }

        memset(input, 0, sizeof(input));
        memset(output, 0, sizeof(output));
    }

    // Close the client socket
    close(client_sock);
    return NULL;
}

void server() {
    printf("You are server\n");

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    pthread_t client_thread_id, input_thread_id;
    socklen_t client_addr_len;
    int opt = 1;

    // Create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server socket options
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind server socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, MAX_LISTENERS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    //Create thread to handle server input
    if (pthread_create(&input_thread_id, NULL, handle_server_input, &server_sock) != 0) {
        perror("Server input thread creation failed");
        exit(EXIT_FAILURE);
    }

    // Main server loop
    while (keep_running) {
        // Accept incoming connection
        client_addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Create thread to handle client communication
        if (pthread_create(&client_thread_id, NULL, handle_client, &client_sock) != 0) {
            perror("Thread creation failed");
            close(client_sock);
            continue;
        }

        // Detach the thread to allow it to run independently
        pthread_detach(client_thread_id);
    }

    // Close the server socket
    close(server_sock);
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
