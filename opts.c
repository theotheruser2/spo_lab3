#include "opts.h"

static char* optstr = "hcsa:p:d:";

void print_help() {
    puts("-h: Help");
    puts("-s: Start the server. Optional -d and -k args.");
    puts("\t-d: Root directory.");
    puts("-c: Start the client mode. Optional -a and -p args.");
    puts("\t-a: IP address of the server.");
    puts("-p: Server port.");
}

void get_options(int argc, char** argv, struct options* options) {
    char client_flag = 0;
    char server_flag = 0;
    int c;
    options->mode = UNDEF;
    while ((c = getopt(argc, argv, optstr)) != -1) {
        switch (c) {
            case 's': server_flag = 1; options->mode = SERVER; break;
            case 'd': strcpy(options->server_params.root_path, optarg); break;
            case 'c': client_flag = 1; options->mode = CLIENT; break;
            case 'a': strcpy(options->client_params.server_ip, optarg); break;
            case 'p': options->server_port = strtol(optarg, NULL, 10); break;
            default: print_help(); exit(404);
        }
    }
    if (client_flag && server_flag || options->mode == UNDEF) {
        print_help();
        exit(404);
    }
}
