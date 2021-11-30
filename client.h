#ifndef LAB3_CLIENT_H
#define LAB3_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <locale.h>

#include "msg.h"

enum client_view {
    DIR_VIEW,
    FILE_VIEW,
    INPUT_VIEW
};

void start_client(char* ip_addr, int port);

#endif
