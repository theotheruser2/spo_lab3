#ifndef LAB3_MSG_H
#define LAB3_MSG_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define DATA_MAX 3000

#define HELLO_MSG   "Greetings"

#define HELP_MSG   "help       - Show the help message\n"                \
                        "pwd        - Current directory\n"               \
                        "cd         - Change directory\n"                       \
                        "ls         - Show files in current directory\n"    \
                        "cat        - Print file content\n"                    \
                        "upload     - Uploads file on server\n"                 \
                        "download   - Downloads file from server\n"             \
                        "exit       - Stops the program\n"

enum command {
    EOSIG = 0,
    HELP,
    CAT,
    UPLOAD,
    DOWNLOAD,
    PWD,
    CD,
    LS,
    EXIT,
    HELLO, // handshake with server
    ERROR, // server error. only from server
    UPDATE // update current directory
};

struct constructed_message {
    enum command command;
    unsigned long long data_length;
    char dir[PATH_MAX];
    char* data;
};

void send_constructed_msg(struct constructed_message* constructed_message, int socket_desc);
struct constructed_message* receive_constructed_msg(int socket_desc);
struct constructed_message* copy_constructed_msg(struct constructed_message* src);
void free_constructed_msg(struct constructed_message* message);


#endif 
