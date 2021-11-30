#include "server.h"

static char root[PATH_MAX] = {'\0'};

static inline size_t dirent_len(struct dirent* dirent) {
    return sizeof(dirent->d_type) +
           sizeof(dirent->d_ino) +
           sizeof(dirent->d_reclen) +
	   sizeof(strlen(dirent->d_name)) +
           strlen(dirent->d_name) + 1;
}

static bool check_dir(char* requested_dir) {
    if (strlen(requested_dir) < strlen(root)) return false;
    return strncmp(requested_dir, root, strlen(root)) == 0;
}

static struct constructed_message* handle_hello_message(struct constructed_message* message) {
    struct constructed_message* output_message = malloc(sizeof(struct constructed_message));
    output_message->command = HELLO;
    output_message->data = malloc(strlen(HELLO_MSG) + 1);
    strcpy(output_message->data, HELLO_MSG);
    output_message->data_length = strlen(HELLO_MSG) + 1;
    strcpy(output_message->dir, root);
    return output_message;
}

static struct constructed_message* handle_help_message(struct constructed_message* message) {
    struct constructed_message* output_message = malloc(sizeof(struct constructed_message));
    output_message->command = HELP;
    output_message->data = malloc(strlen(HELP_MSG) + 1);
    strcpy(output_message->data, HELP_MSG);
    output_message->data_length = strlen(HELP_MSG) + 1;
    return output_message;
}

static struct constructed_message* handle_pwd_message(struct constructed_message* message) {
    struct constructed_message* output_message = malloc(sizeof(struct constructed_message));
    strcpy(output_message->dir, message->dir);
    output_message->command = PWD;
    output_message->data_length = strlen(message->dir);
    output_message->data = malloc(strlen(message->dir));
    strncpy(output_message->data, message->dir, output_message->data_length);
    return output_message;
}

static struct constructed_message* handle_cd_message(struct constructed_message* message) {
    struct constructed_message* output_message = calloc(1, sizeof(struct constructed_message));
    output_message->data_length = 0;
    char real_path[PATH_MAX] = {'\0'};
    char selected_dir[PATH_MAX] = {'\0'};
    strncpy(selected_dir, message->data, message->data_length);
    if (selected_dir[0] == '/') {
        realpath(selected_dir, real_path);
    } else {
        strcat(selected_dir, message->dir);
        strncpy(selected_dir, message->data, message->data_length);
        realpath(selected_dir, real_path);
    }
    if (check_dir(real_path)) {
        output_message->command = CD;
        strcpy(output_message->dir, real_path);
    } else {
        output_message->command = ERROR;
    }
    return output_message;
}

static struct constructed_message* handle_ls_message(struct constructed_message* message) {
    struct constructed_message* output_message = malloc(sizeof(struct constructed_message));
    DIR* dir;
    struct dirent* entry;
    size_t dirent_size;
    size_t data_ptr = 0;
    output_message->data_length = 0;
    output_message->data = malloc(1);

    strcpy(output_message->dir, message->dir);

    if (check_dir(message->dir)) {
        output_message->command = LS;
        dir = opendir(message->dir);
        if (!dir) {
            output_message->command = ERROR;
            return output_message;
        }
        while ((entry = readdir(dir)) != NULL) {
            dirent_size = dirent_len(entry);
            data_ptr = output_message->data_length;
            output_message->data_length += dirent_size;
            output_message->data = realloc(output_message->data, output_message->data_length);
            memcpy(output_message->data + data_ptr, entry, dirent_size);
        }
    } else {
        output_message->command = ERROR;
    }
    return output_message;
}

static struct constructed_message* handle_cat_message(struct constructed_message* message) {
    struct constructed_message* output_message = malloc(sizeof(struct constructed_message));
    char filename[PATH_MAX] = {'\0'};
    char buffer[DATA_MAX] = {'\0'};
    size_t buf_len;
    FILE* file;

    output_message->data_length = 0;
    output_message->data = malloc(1);

    if (message->data[0] == '/') {
        strncpy(filename, message->data, message->data_length);
    } else {
        strcat(filename, message->dir);
        strncat(filename, message->data, message->data_length);
    }
    if (check_dir(filename)) {
        output_message->command = CAT;
        file = fopen(filename, "rb");
        while (fread(buffer, DATA_MAX, 1, file) > 0) {
            output_message->data = realloc(output_message->data, output_message->data_length + DATA_MAX);
            strncpy(output_message->data + output_message->data_length, buffer, DATA_MAX);
            output_message->data_length += DATA_MAX;
        }
        buf_len = strlen(buffer);
        output_message->data = realloc(output_message->data, output_message->data_length + buf_len);
        strncpy(output_message->data + output_message->data_length, buffer, buf_len);
        output_message->data_length += buf_len;
        fclose(file);
    } else {
        output_message->command = ERROR;
    }
    return output_message;
}

static struct constructed_message* handle_upload_message(struct constructed_message* message) {
    struct constructed_message* output_message = malloc(sizeof(struct constructed_message));
    char buffer[DATA_MAX] = {'\0'};
    unsigned long long data_ptr = 0;
    FILE* file = fopen(message->dir, "wb");

    if (check_dir(message->dir) && file != NULL) {

        while(data_ptr < message->data_length) {
            strncpy(buffer, message->data + data_ptr, MIN(DATA_MAX, message->data_length));
            fwrite(buffer, MIN(DATA_MAX, message->data_length), 1, file);
            data_ptr += DATA_MAX;
        }
        output_message->command = UPDATE;

        fclose(file);
    } else {
        output_message->command = ERROR;
    }

    return output_message;
}

static struct constructed_message* handle_download_message(struct constructed_message* message) {
    struct constructed_message* output_message = malloc(sizeof(struct constructed_message));
    char filename[PATH_MAX] = {'\0'};
    char buffer[DATA_MAX] = {'\0'};
    size_t buf_len;
    FILE* file;

    output_message->data_length = 0;
    output_message->data = malloc(1);

    if (message->data[0] == '/') {
        strncpy(filename, message->data, message->data_length);
    } else {
        strcat(filename, message->dir);
        strncat(filename, message->data, message->data_length);
    }

    if (check_dir(filename)) {
        output_message->command = DOWNLOAD;
        file = fopen(filename, "rb");
        while (fread(buffer, DATA_MAX, 1, file) > 0) {
            output_message->data = realloc(output_message->data, output_message->data_length + DATA_MAX);
            strncpy(output_message->data + output_message->data_length, buffer, DATA_MAX);
            output_message->data_length += DATA_MAX;
        }
        buf_len = strlen(buffer);
        output_message->data = realloc(output_message->data, output_message->data_length + buf_len);
        strncpy(output_message->data + output_message->data_length, buffer, buf_len);
        output_message->data_length += buf_len;
        fclose(file);
    } else {
        output_message->command = ERROR;
    }
    return output_message;
}


static struct constructed_message* handle_constructed_msg(struct constructed_message* message) {
    switch (message->command) {
        case PWD:
            return handle_pwd_message(message);
        case CD:
            return handle_cd_message(message);
        case LS:
            return handle_ls_message(message);
        case CAT:
            return handle_cat_message(message);
        case UPLOAD:
            return handle_upload_message(message);
        case DOWNLOAD:
            return handle_download_message(message);
        case HELLO:
            return handle_hello_message(message);
        case EXIT:
            exit(404);
    }
    return handle_help_message(message);
}

void* exit_handler(void* socket_desc) {
    char input;
    do {
        scanf("%c", &input);
    } while (input != EXIT_CHAR);
    close(*(int*) socket_desc);
    exit(0);
}

void start_server(char* root_dir, int port) {
    int listenfd, c, new_socket;
    int* server_socket = malloc(1);
    struct sockaddr_in server, client;
    struct constructed_message *input_message, *output_message;
    fd_set active_fd_set, read_fd_set;
    pthread_t exit_thread;

    signal(SIGPIPE, SIG_IGN);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Could not create socket.");
        return;
    }

    if ((bind(listenfd, (struct sockaddr*) &server, sizeof(server))) < 0) {
        perror("Could not bind server.");
        return;
    }

    listen(listenfd, MAX_BACKLOG);

    strcpy(root, root_dir);

    *server_socket = listenfd;

    if (pthread_create(&exit_thread, NULL, exit_handler, (void*) server_socket) != 0) {
        perror("Couldn't create exit thread.");
        return;
    }

    FD_ZERO(&active_fd_set);
    FD_SET(listenfd, &active_fd_set);

    printf("Server started on port: %u. Root directory: %s\n", port, root_dir);

    c = sizeof(struct sockaddr_in);

    while (true) {
        read_fd_set = active_fd_set;

        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("select()");
            return;
        }

        for (int i = 0; i < 10; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == listenfd) {
                    new_socket = accept(listenfd, (struct sockaddr*) &client, (socklen_t* )&c);
                    if (new_socket < 0) {
                        perror("accept()");
                        return;
                    }
                    FD_SET(new_socket, &active_fd_set);
                    puts("Connection accepted");
                    input_message = receive_constructed_msg(new_socket);
                    output_message = handle_constructed_msg(input_message);

                    send_constructed_msg(output_message, new_socket);

                    free_constructed_msg(input_message);
                    free_constructed_msg(output_message);
                } else {
                    input_message = receive_constructed_msg(i);
                    if (input_message->command != EOSIG) {
                        output_message = handle_constructed_msg(input_message);

                        if (input_message->command == UPLOAD && output_message->command == UPDATE) {
                            for (int j = 3; j < 10; ++j) {
                                send_constructed_msg(output_message, j);
                            }
                        } else {
                            send_constructed_msg(output_message, i);
                        }

                        free_constructed_msg(output_message);
                    } else {
                        close(i);
                        FD_CLR(i, &active_fd_set);
                    }
                    free_constructed_msg(input_message);
                }
            }
        }
    }
}
