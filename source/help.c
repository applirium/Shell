#include "../header/help.h"

char *builtin_str[] ={"help","stat"};

//help function to print information about program
void help() {
    printf("Author - Lukáš Štefančík\n"
           "SPAASM Zadanie 2\n"
           "-h for printing help\n"
           "-s for switching into server\n"
           "-c for switching into client\n"
           "-p <port number> to set port\n"
           "-i <ip> to set IP adress\n"
           ";l <file> to set history file"
           "\n\n"
           "Connection modes\n"
           "    0 - SERVER\n"
           "    1 - CLIENT\n"
           "    2 - LOCAL\n"
           "\n"
           "Shell uses these builtins:\n"
           "    halt\n"
           "    quit\n"
           "    abort\n");

    for (int i = 0; i < sizeof(builtin_str) / sizeof(char *); i++) {
        printf("    %s\n", builtin_str[i]);
    }
}