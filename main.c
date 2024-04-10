#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_LISTENERS 5
#define SERVER 0
#define CLIENT 1
#define LOCAL 2
#define BUFFER_SIZE 65536

int mode = LOCAL;
int port = 1234;

void client()
{
    printf("You are client\n");
}

void server()
{
    printf("You are server\n");
}

void local()
{
    printf("You are local\n");
}

int main(int argc, char **argv) {
    int opt;

    while ((opt = getopt(argc, argv, ":hscp:p:")) != -1)
    {
        switch(opt) {
            case 'h':
                printf("Author - Lukáš Štefančík\n"
                       "SPAASM Zadanie 2\n"
                       "-h for printing help\n"
                       "-s for switching into server\n"
                       "-c for switching into client\n"
                       "-p <port number> to set port\n"
                       "\n"
                );
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
