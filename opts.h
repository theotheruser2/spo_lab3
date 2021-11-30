#ifndef LAB3_OPTS_H
#define LAB3_OPTS_H

#include <stdio.h>
#include <limits.h>
#include <inttypes.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#define MAX_IPV4_LENGTH 16

enum mode {
    UNDEF = 0,
    CLIENT,
    SERVER
};

struct server_params {
    char root_path[PATH_MAX];
};

struct client_params {
    char server_ip[MAX_IPV4_LENGTH];
};

struct options {
    enum mode mode;
    int server_port;
    union {
        struct server_params server_params;
        struct client_params client_params;
    };
};

void get_options(int argc, char** argv, struct options* options);

#endif
