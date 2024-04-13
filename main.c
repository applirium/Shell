#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pwd.h>

#define MAX_LISTENERS 5
#define SERVER 0
#define CLIENT 1
#define BUFFER_SIZE 65536
#define NAME_SIZE 20

// Main Program                6
// 3. stat                   + 3
// 7. Inline assembler       + 3
// 9. Client                 + 3
// 10. Special characters '' + 2
// 11. IP -i                 + 2
// 24. logs -l               + 2
// 28. TODO Makefile
// 30. English Documentation + 1
// ---------------------------
//                   6 + 16 = 22

char pathname[50];
int mode = -1;
int port = 12345;
char ip[16];

char file[NAME_SIZE];
int file_flag = 0;

int builtin_help(void *args);
int builtin_stat(void *args);
int (*builtin_func[]) (void *) = {&builtin_help,&builtin_stat};
char *builtin_str[] ={"help","stat"};

typedef struct Connection {
    int mode;
    int socket;
    int std_in;
    int std_out;
    struct sockaddr_in client_addr;
    int* clients;
} Connection;

typedef struct Node {
    char** command;
    struct Node* next;
} Node;

void help()
{
    printf("Author - Lukáš Štefančík\n"
           "SPAASM Zadanie 2\n"
           "-h for printing help\n"
           "-s for switching into server\n"
           "-c for switching into client\n"
           "-p <port number> to set port\n"
           "-i <ip> to set IP adress\n"
           ";l <file> to set history file"
           "\n"
           "Connection modes\n"
           "    0 - SERVER\n"
           "    1 - CLIENT\n"
           "    2 - LOCAL\n"
           "\n"
           "Shell uses these builtins:\n");

    for (int i = 0; i < sizeof(builtin_str) / sizeof(char *); i++) {
        printf("    %s\n", builtin_str[i]);
    }
}

int builtin_help(void *)
{
    help();
    return 0;
}

int builtin_stat(void *args) {
    Connection *arg = (Connection *)args;

    if(arg->mode == CLIENT)
    {
        printf("Používaný port je: %d Používaná ip adresa je %s\n", port,ip);
    }
    else
    {
        printf("Používaný port je: %d Používaná ip adresa je %s\n", port,ip);
        for(int i = 0; i < MAX_LISTENERS; i++)
        {
            if (arg->clients[i] == 0) break;
            printf("    Reserverd socket - client %d: %d\n", i+1, arg->clients[i]);
        }
    }
    return 0;
}


// Function to create a new node
struct Node* createNode(char** data)
{
    struct Node *newNode = (struct Node *) malloc(sizeof(struct Node));

    newNode->command = (char **) malloc(sizeof(char *) * 10);
    for (int i = 0; i < 10; i++)
    {

        if (data[i] != NULL)
        {
            // Allocate memory for the string and copy data[i] to it
            newNode->command[i] = (char *) malloc(strlen(data[i]));
            newNode->command[i] = strdup(data[i]);
        } else
        {
            // If data[i] is NULL, set newNode->command[i] to NULL
            newNode->command[i] = NULL;
        }
    }

    newNode->next = NULL;
    return newNode;
}

// Function to insert a node at the end of the linked list
void insertAtEnd(struct Node** headRef, char** data) {
    struct Node* newNode = createNode(data);
    struct Node* last = *headRef;

    if (*headRef == NULL)
    {
        *headRef = newNode;
        return;
    }

    while (last->next != NULL)
    {
        last = last->next;
    }
    last->next = newNode;
}

// Function to delete the entire linked list
void deleteList(struct Node** headRef)
{
    struct Node* current = *headRef;
    struct Node* next;

    while (current != NULL)
    {
        next = current->next;
        free(*current->command); // Free the memory allocated for the command string
        free(current->command); // Free the memory allocated for the command array
        free(current);
        current = next;
    }
    *headRef = NULL; // Update head to NULL after deleting all nodes
}

Node* input_parsing(char* input)
{
    Node* linked_list = NULL;
    char* command_pieces[10] = {NULL}; // Array to hold command pieces
    int index = 0, piece_index = 0, special = 0;

    for (int i = 0; i <= strlen(input); i++)
    {
        if (input[i] == ' ')        // Check for space or end of string
        {
            if(index != 0)
            {
                command_pieces[piece_index][index] = '\0';
                piece_index++; // Increment piece_index when we encounter space or end of string
                index = 0; // Reset index for the next command piece
            }
        }

        else if ((input[i] == '#' || input[i] == '>' || input[i] == '<' || input[i] == ';' || input[i] == '|' || input[i] == '\0') && special == 0)
        {
            if (piece_index != 0 || index != 0)
            {
                if(command_pieces[piece_index] != NULL)
                {
                    if(command_pieces[piece_index][index] != '\0')
                    {
                        command_pieces[piece_index][index++] = '\0';
                    }
                }

                insertAtEnd(&linked_list, command_pieces); // Insert the command pieces into the linked list
                piece_index = 0; // Reset piece_index
                index = 0;

                for (int j = 0; j < 10; j++)
                {
                    if (command_pieces[j] == NULL)
                    {
                        break;
                    }

                    command_pieces[j] = NULL;
                }
            }

            if(input[i] != '\0')
            {
                char special_char[2] = { input[i], '\0' };
                command_pieces[0] = strdup(special_char);
                command_pieces[1] = NULL;
                insertAtEnd(&linked_list, command_pieces);

                command_pieces[0] = NULL;
            }
        }
        else
        {
            if (command_pieces[piece_index] == NULL)
            {
                command_pieces[piece_index] = (char *) malloc((strlen(input) - i + 1));
            }

            if(input[i] == '\'')
            {
                special = !special;
                continue;
            }

            if(input[i] == '\"' && special != 1)
            {
                continue;
            }

            if(input[i] == '\\' && special != 1)
            {
                i++;
            }

            if(i == strlen(input))
            {
                command_pieces[piece_index][index++] = '\0';
                insertAtEnd(&linked_list, command_pieces);
            }
            else
            {
                command_pieces[piece_index][index++] = input[i];
            }
        }
    }

    return linked_list;
}

void execute(char* input, int std_in, int std_out, int i, Connection* information)
{
    int fd[2], server_stdout, builtin, skip;
    char output[BUFFER_SIZE];
    ssize_t bytes_read;

    // setting output to temporary file
    server_stdout = open("server_output.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);

    Node* parsed_linked_input = input_parsing(input);
    Node* current = parsed_linked_input;

    while (current != NULL)
    {
        skip = 0;

        for(int j = 0; j < sizeof(builtin_str) / sizeof(char *); j++)
        {
            builtin = 0;
            if (strcmp(current->command[0], builtin_str[j]) == 0)
            {
                dup2(server_stdout, STDOUT_FILENO);
                builtin_func[j](information);
                builtin = 1;
                break;
            }
        }
        if (builtin)
        {
            break;
        }

        if (pipe(fd) < 0)
        {
            perror("Pipe error");
        }

        if (strcmp(current->command[0], "#") == 0)
        {
            break;
        }

        if (strcmp(current->command[0], ";") == 0 || strcmp(current->command[0], "|") == 0)
        {
            skip = 1;
        }

        if(!skip)
        {
            if (fork() == 0)
            {
                if (current->next != NULL && strcmp(current->next->command[0], "|") == 0)
                {
                    close(fd[0]);
                    dup2(fd[1], STDOUT_FILENO);
                    dup2(fd[1], STDERR_FILENO);
                }
                else
                {
                    dup2(server_stdout, STDOUT_FILENO);
                    dup2(server_stdout, STDERR_FILENO);
                }

                if (execvp(current->command[0], current->command) == -1)
                {
                    perror("Exec Error");            // if something bad happens call error
                    _exit(1);
                }
            }
            wait(NULL);                 // parent process, child will not get here!!!, wait for child to complete

            close(fd[1]);
            if (current->next != NULL && strcmp(current->next->command[0], "|") == 0)
            {
                dup2(fd[0], STDIN_FILENO);
            }
            close(fd[0]);
        }
        current = current->next;
    }

    deleteList(&parsed_linked_input);
    close(server_stdout);

    dup2(std_in, STDIN_FILENO);
    dup2(std_out, STDOUT_FILENO);

    server_stdout = open("server_output.tmp", O_RDONLY, 0666);
    bytes_read = read(server_stdout, output, sizeof(output) - 1);
    output[bytes_read] = '\0';

    if (i == SERVER)
    {
        printf("%s", output);
    }

    else
    {
        write(information->socket, output, bytes_read);
    }

    close(server_stdout);
    remove("server_output.tmp");
}

// Function to get actual time and return it as a string
char *getTime()
{
    time_t mytime = time(NULL);
    char * time_str = ctime(&mytime);
    time_str[strlen(time_str)-1] = '\0';

    return time_str;
}

char* getServername()
{
    struct passwd *pws;
    int id;
    static char meno[100] = "";

    asm volatile
    (
            "mov $102, %%rax\n"  // Set rax register to 102 (syscall number for getuid)
            "syscall\n"          // Trigger the syscall
            : "=a" (id)          // Output: id
    );

    pws = getpwuid(id);
    strcpy(meno, pws->pw_name);
    return meno;
}

// Function to get hostname and return it as a string
char *getHostname()
{
    static char hostname[NAME_SIZE]; // Assuming maximum hostname length is 255 characters
    // Get the hostname
    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        perror("gethostname failed");
        return NULL;
    }
    return hostname;
}

// Function is adding a new sockets to array of all active sockets when connected
int connection(int** client_connections, int socket)
{
    for(int i = 0; i < MAX_LISTENERS; i++)
    {
        if((*client_connections)[i] == 0)
        {
            (*client_connections)[i] = socket;
            return 0;
        }
    }
    return -1;
}

// Function is deleting a sockets where are disconnecting from array of all active sockets
int disconnection(int** client_connections, int socket)
{
    for(int i = 0; i < MAX_LISTENERS; i++)
    {
        if((*client_connections)[i] == socket)
        {
            (*client_connections)[i] = 0;
            return 0;
        }
    }
    return -1;
}

//Function to handle client
void client()
{
    printf("You are client\n");

    int sock, connection_status, log_file;                                     // Definitions of variables
    struct sockaddr_in connection_client;
    char output[BUFFER_SIZE], input[BUFFER_SIZE], name[NAME_SIZE];

    //Handeling client's connection
    sock = socket(AF_INET, SOCK_STREAM, 0);     // Create client socket
    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    connection_client.sin_family = AF_INET;                         // Set server address structure
    connection_client.sin_port = htons(port);
    connection_client.sin_addr.s_addr = INADDR_ANY;

    if (connect(sock, (struct sockaddr *) &connection_client, sizeof(connection_client)) < 0)       // Connect to the server
    {
        perror("Socket connection failed");
        exit(EXIT_FAILURE);
    }

    // Client name definition
    printf("Enter your client name: ");
    fgets(name, sizeof(name), stdin);        // Read input from the user using fgets

    size_t name_length = strlen(name);                 // Properly ending a string with \0
    if (name_length > 0 && name[name_length - 1] == '\n')
    {
        name[name_length - 1] = '\0';
    }

    write(sock, name, strlen(name));        // Sending server a name of new client
    read(sock, output, sizeof(output));    // Reading server anwere of allocation to server listeners
    sscanf(output, "%d", &connection_status);

    if(connection_status < 0)       // If > 0 succeffull, If < 0 full server, recejcted
    {
        printf("Can't connect client, exceeding max number of listeners\n");
        close(sock);
        pthread_exit(NULL);     // Shut down of client
    }

    if(file_flag)
    {
        log_file = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if((log_file) < 0)
        {
            file_flag = 0;
            perror("Log Error");
        }
    }

    // Handle client communication
    while(1)
    {
        memset(input, 0, sizeof(input));            // Reset of buffers
        memset(output, 0, sizeof(output));

        printf("%s %s@%s# ",getTime(), name, getHostname());

        fgets(input, BUFFER_SIZE, stdin);        // Read input from stdin

        if (file_flag)
        {
            write(log_file, input, strlen(input));
        }

        input[strlen(input) - 1] = '\0';

        if(strcmp(input,"") != 0)
        {
            write(sock, input, strlen(input));      // Send input to the server
            read(sock, output, sizeof(output));    // Receive output from the server
        }

        if (strcmp(output, "halt") == 0)          // If server will quit
        {
            printf("Autodisconnection - Server disconnected\n");
            close(file_flag);
            close(sock);
            break;
        }

        if (strcmp(input, "quit") == 0)           // If client will quit
        {
            close(file_flag);
            close(sock);
            break;
        }

        printf("%s", output);        // Print the received output
    }
}

//Function is defined as server's thread whitch enable server stdin input
void *handle_server_input(void *arg)
{
    int log_file;
    Connection *args = (Connection *)arg;            // Definitions of variables
    char output[BUFFER_SIZE], input[BUFFER_SIZE];

    if(file_flag)
    {
        log_file = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if((log_file) < 0)
        {
            file_flag = 0;
            perror("Log Error");
        }
    }

    while (1)
    {
        memset(input, 0, sizeof(input));            // Reset of buffers
        memset(output, 0, sizeof(output));

        printf("%s %s@%s# ",getTime(), getServername(), getHostname());
        fgets(input, BUFFER_SIZE, stdin);       // Read input from stdin

        if (file_flag)
        {
            write(log_file, input, strlen(input));
        }

        input[strlen(input) - 1] = '\0';

        if (strcmp(input, "halt") == 0)                    //if input == halt breaks
        {
            int* clients = args->clients;
            for(int i = 0; i < MAX_LISTENERS; i++)
            {
                if (clients[i] != 0)
                {
                    write(clients[i], input, strlen(input));
                }
                close(file_flag);
            }
            exit(EXIT_SUCCESS);
        }
        else                                             //else execute command
        {
            execute(input, args->std_in, args->std_out, SERVER, (void*)args);     //copy bash output to output buffer
        }
    }
}

//Function is defined as server's thread whitch are handeling each one connection to server
void *handle_client(void *arg)
{
    Connection *args = (Connection *)arg;                // Definitions of variables
    struct sockaddr_in client_addr = args->client_addr;
    int client_sock = args->socket, connection_status;
    char input[BUFFER_SIZE], output[BUFFER_SIZE], name[NAME_SIZE];

    // New connection approval process
    read(client_sock, name, sizeof(name));        // Reading name of new connection
    connection_status = connection(&args->clients, client_sock);    // Checking if server isn't full

    sprintf(output, "%d",connection_status);
    write(client_sock, output, strlen(output));         // Returning answer to client

    if(connection_status < 0)                                      // If > 0 succeffull, If < 0 full server, recejcted
    {
        close(client_sock);
        pthread_exit(NULL);             // Shut down of server's thread to handle new client
    }

    printf("Client %s connected: %s:%d\n",name, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    while (1)
    {
        memset(input, 0, sizeof(input));
        memset(output, 0, sizeof(output));

        read(client_sock, input, sizeof(input));    // Receive output from the client
        if (strcmp(input, "quit") == 0)                          // Check if the client wants to quit
        {
            printf("Client %s disconnected %s:%d\n",name, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            disconnection(&args->clients, client_sock);
            break;
        }
        else
        {
            execute(input, args->std_in, args->std_out,  CLIENT, (void*)args);                     // Execute input and copy it to output
        }
    }

    close(client_sock);          // Close the client socket
    pthread_exit(NULL);
}

//Function to handle server
void server()
{
    printf("You are server\n");

    int std_in, std_out, server_sock, client_sock, opt = 1;                               // Definitions of variables
    struct sockaddr_in server_addr, client_addr;
    pthread_t client_thread_id, input_thread_id;
    socklen_t client_addr_len;

    std_in = dup(0);
    std_out = dup(1);

    int server_connection[MAX_LISTENERS];
    for(int i = 0; i < MAX_LISTENERS; i++)
    {
        server_connection[i] = 0;
    }

    //Handeling client's connection
    server_sock = socket(AF_INET, SOCK_STREAM, 0);      // Create server socket
    if (server_sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)       // Set server socket options
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));               // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)            // Bind server socket
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, MAX_LISTENERS) < 0)             // Listen for incoming connections
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    Connection arguments_server = {SERVER,server_sock, std_in, std_out, server_addr, server_connection};
    if (pthread_create(&input_thread_id, NULL, handle_server_input, &arguments_server) != 0)     //Create thread to handle input on server
    {
        perror("Server input thread creation failed");
        exit(EXIT_FAILURE);
    }

    // Main server loop
    while (1)
    {
        client_addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);       // Accept incoming connection

        if(client_sock < 0)
        {
            perror("Accept failed");
            continue;
        }

        Connection arguments = {CLIENT,client_sock, std_in, std_out, client_addr, server_connection};        // Pushing information to threads
        if (pthread_create(&client_thread_id, NULL, handle_client, &arguments) != 0)                // Create thread to handle client communication
        {
            perror("Thread creation failed");
            close(client_sock);
            continue;
        }

        pthread_detach(client_thread_id);           // Detach the thread to allow it to run independently
    }
}

int main(int argc, char **argv)
{
    int opt;
    strcpy(pathname, "./sck");
    strcpy(ip, "127.0.0.1");

    while ((opt = getopt(argc, argv, ":hsci:l:p:")) != -1)
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
            case 'i':
                strcpy(ip, optarg);
                break;

            case 'l':
                strcpy(file, optarg);
                file_flag = 1;
                break;

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
