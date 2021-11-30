#include "client.h"

static char current_path[PATH_MAX] = {'\0'};
static char downloaded_filename[PATH_MAX] = {'\0'};
static char last_notification[DATA_MAX] = {'\0'};
struct constructed_message* last_ls_message;
int selected_element;

static void print_menu() {
    printw("To navigate up/down use 'w'/'s' keys.\n");
    printw("To download file use the 'd' key.\n");
    printw("To upload a file press the 'u' key and enter the filename.\n");
    printw("To go into directory / view file press 'e' key.\n");
    printw("To exit press the 'q' key.\n");
    if (last_notification[0] != '\0') {
        printw("\nNotification: %s\n\n", last_notification);
    } else {
        printw("\n\n\n");
    }
    refresh();
}

static inline size_t dirent_len(struct dirent* dirent) {
    return sizeof(dirent->d_type) +
           sizeof(dirent->d_ino) +
           sizeof(dirent->d_reclen) +
	   sizeof(strlen(dirent->d_name)) +
           strlen(dirent->d_name) + 1;
}

static void hello_message(int socket_desc) {
    struct constructed_message* hello_msg = malloc(sizeof(struct constructed_message));
    struct constructed_message* resp_msg;
    hello_msg->command = HELLO;

    send_constructed_msg(hello_msg, socket_desc);
    resp_msg = receive_constructed_msg(socket_desc);

    if (strncmp(resp_msg->data, HELLO_MSG, strlen(HELLO_MSG)) != 0) {
        printf("Unexpected message from the server.\n");
        return;
    }

    strcpy(current_path, resp_msg->dir);
    free(hello_msg);
    free_constructed_msg(resp_msg);
}

static void send_ls_message(int socket_desc, char* path) {
    struct constructed_message* ls_msg = malloc(sizeof(struct constructed_message));

    ls_msg->command = LS;
    strcpy(ls_msg->dir, path);
    ls_msg->data_length = 0;
    ls_msg->data = NULL;
    send_constructed_msg(ls_msg, socket_desc);
    free(ls_msg);
}

static void send_cd_message(int socket_desc, char* path) {
    struct constructed_message* cd_msg = calloc(1, sizeof(struct constructed_message));

    cd_msg->command = CD;
    cd_msg->data_length = strlen(current_path) + strlen(path) + 2;
    cd_msg->data = calloc(1, cd_msg->data_length);

    strcpy(cd_msg->dir, current_path);
    strcpy(cd_msg->data, current_path);
    strcat(cd_msg->data, "/");
    strcat(cd_msg->data, path);

    send_constructed_msg(cd_msg, socket_desc);
    free_constructed_msg(cd_msg);
}

static void send_cat_message(int socket_desc, char* path) {
    struct constructed_message* cat_msg = calloc(1, sizeof(struct constructed_message));

    cat_msg->command = CAT;
    cat_msg->data_length = strlen(current_path) + strlen(path) + 2;
    cat_msg->data = calloc(1, cat_msg->data_length);

    strcpy(cat_msg->dir, current_path);
    strcpy(cat_msg->data, current_path);
    strcat(cat_msg->data, "/");
    strcat(cat_msg->data, path);

    send_constructed_msg(cat_msg, socket_desc);
    free_constructed_msg(cat_msg);
}

static void send_download_message(int socket_desc, char* filename) {
    struct constructed_message* dl_msg = calloc(1, sizeof(struct constructed_message));

    dl_msg->command = DOWNLOAD;
    dl_msg->data_length = strlen(current_path) + strlen(filename) + 2;
    dl_msg->data = calloc(1, dl_msg->data_length);

    strcpy(dl_msg->dir, current_path);
    strcpy(dl_msg->data, current_path);
    strcat(dl_msg->data, "/");
    strcat(dl_msg->data, filename);

    send_constructed_msg(dl_msg, socket_desc);
    free_constructed_msg(dl_msg);
}

static void send_upload_message(int socket_desc, char* filename) {
    struct constructed_message* upload_msg = calloc(1, sizeof(struct constructed_message));
    char buffer[DATA_MAX] = {'\0'};
    char* filename_ptr = filename + strlen(filename);
    size_t buf_len;

    FILE* file = fopen(filename, "rb");

    upload_msg->data = NULL;

    if (file != NULL) {
        upload_msg->command = UPLOAD;
        strcpy(upload_msg->dir, current_path);
        strcat(upload_msg->dir, "/");
        // change filename. Save only from last /
        while (*--filename_ptr != '/') {}
        *filename_ptr = '\0';
        strcat(upload_msg->dir, ++filename_ptr);

        upload_msg->data_length = 0;
        upload_msg->data = malloc(1);

        while (fread(buffer, DATA_MAX, 1, file) > 0) {
            upload_msg->data = realloc(upload_msg->data, upload_msg->data_length + DATA_MAX);
            strncpy(upload_msg->data + upload_msg->data_length, buffer, DATA_MAX);
            upload_msg->data_length += DATA_MAX;
        }
        buf_len = strlen(buffer);
        upload_msg->data = realloc(upload_msg->data, upload_msg->data_length + buf_len);
        strncpy(upload_msg->data + upload_msg->data_length, buffer, buf_len);
        upload_msg->data_length += buf_len;

        send_constructed_msg(upload_msg, socket_desc);
        fclose(file);
    } else {
        strcpy(last_notification, "Could not open file ");
        strcat(last_notification, filename);
    }

    //
    free_constructed_msg(upload_msg);
}

static struct dirent* get_selected_dirent() {
    size_t data_ptr = 0, i = 0;
    struct dirent* element = malloc(sizeof(struct dirent));

    while(data_ptr < last_ls_message->data_length) {
        memcpy(element, last_ls_message->data + data_ptr, sizeof(struct dirent));
        if (i++ == selected_element) {
            return element;
        }

        data_ptr += dirent_len(element);
    }
    return NULL;
}

static char* get_selected_name() {
    struct dirent* entry = get_selected_dirent();
    char* name = calloc(1, strlen(entry->d_name) + (entry->d_type == DT_DIR ? 2 : 1));
    strcpy(name, entry->d_name);
    if (entry->d_type == DT_DIR) {
        strcat(name, "/");
    }
    free(entry);
    return name;
}

static unsigned char get_selected_type() {
    struct dirent* element = get_selected_dirent();
    unsigned char type = element->d_type;
    free(element);
    return type;
}

static void print_directory(int step) {
    size_t data_ptr = 0, i = 0;
    struct dirent* element = malloc(sizeof(struct dirent));
    bool printed_selected = false;

    selected_element += step;

    clear();
    print_menu();

    if (selected_element < 0) {
        selected_element = 0;
    }

    while(data_ptr < last_ls_message->data_length) {
        memcpy(element, last_ls_message->data + data_ptr, sizeof(struct dirent));
        if (i++ == selected_element) {
            printw("> %s", element->d_name);
            printed_selected = true;
        } else {
            printw("  %s", element->d_name);
        }

        if (element->d_type == DT_DIR) {
            printw("/\n");
        } else {
            printw("\n");
        }
        data_ptr += dirent_len(element);
    }

    free(element);

    if (printed_selected == false) {
        print_directory(-1);
    }

    refresh();
}

static void print_file(struct constructed_message* file_message) {
    file_message->data[file_message->data_length] = '\0';
    clear();
    printw("To return 'r'\n\n");
    printw("%s", file_message->data);
    refresh();
}

static void save_file(struct constructed_message* file_message) {
    FILE* file;
    char filename[PATH_MAX] = {'\0'};
    char buffer[DATA_MAX] = {'\0'};
    unsigned long long data_ptr = 0;

    strcpy(filename, getenv("HOME"));
    strcat(filename, "/Downloads/");
    strcat(filename, downloaded_filename);
    file = fopen(filename, "wb");

    if (file != NULL) {

        while (data_ptr < file_message->data_length) {
            strncpy(buffer, file_message->data + data_ptr, MIN(DATA_MAX, file_message->data_length));
            fwrite(buffer, MIN(DATA_MAX, file_message->data_length), 1, file);
            data_ptr += DATA_MAX;
        }

        strcpy(last_notification, "File ");
        strcat(last_notification, downloaded_filename);
        strcat(last_notification, " was saved as ");
        strcat(last_notification, filename);

        fclose(file);
    } else {
        endwin();
        puts(filename);
        puts("Could not download the file.");
        exit(EXIT_FAILURE);
    }
}

static void start_screen_handler(int socket_desc) {
    int c, i;
    char* name;
    char inputFilename[PATH_MAX] = {'\0'};
    enum client_view client_view = DIR_VIEW;

    do {
        c = getch();

        if (client_view == INPUT_VIEW) {
            i = 0;
            while (c != '\n') {
                inputFilename[i++] = c;
                c = getch();
            }
            inputFilename[PATH_MAX - 1] = '\0';
            inputFilename[i] = '\0';
            send_upload_message(socket_desc, inputFilename);
            client_view = DIR_VIEW;
            noecho();
            cbreak();
            print_directory(0);
            continue;
        }

        switch (c) {
            case 'r':
                if (client_view == FILE_VIEW) {
                    send_ls_message(socket_desc, current_path);
                    client_view = DIR_VIEW;
                }
                break;
            case 'w':
                if (client_view == DIR_VIEW) {
                    print_directory(-1);
                }
                break;
            case 's':
                if (client_view == DIR_VIEW) {
                    print_directory(1);
                }
                break;
            case 'd':
                if (client_view == DIR_VIEW) {
                    name = get_selected_name();
                    if (get_selected_type() == DT_REG) {
                        strcpy(downloaded_filename, name);
                        send_download_message(socket_desc, name);
                    }
                    free(name);
                }
                break;
            case 'u':
                if (client_view == DIR_VIEW) {
                    client_view = INPUT_VIEW;
                    echo();
                    nocbreak();
                }
                break;
            case 'e':
                if (client_view == DIR_VIEW) {
                    name = get_selected_name();
                    if (get_selected_type() == DT_DIR) {
                        send_cd_message(socket_desc, name);
                    } else {
                        send_cat_message(socket_desc, name);
                        client_view = FILE_VIEW;
                    }
                    free(name);
                }
                break;
        }
    } while (c != 'q');
}

void* receive_message_handler(void* socket_desc) {
    int socket = *(int*) socket_desc;
    struct constructed_message* recv_message = NULL;
    do {
        free_constructed_msg(recv_message);
        recv_message = receive_constructed_msg(socket);
        switch (recv_message->command) {
            case CD:
                strcpy(current_path, recv_message->dir);
                send_ls_message(socket, current_path);
                break;
            case LS:
                last_ls_message = copy_constructed_msg(recv_message);
                selected_element = 0;
                print_directory(0);
                break;
            case CAT:
                print_file(recv_message);
                break;
            case DOWNLOAD:
                save_file(recv_message);
                print_directory(0);
                break;
            case UPDATE:
                send_ls_message(socket, current_path);
                break;
            case EXIT:
                endwin();
                puts("Server closed");
                close(socket);
                exit(0);
            case ERROR:
                // endwin and exit
                break;
            case HELLO:
            case UPLOAD:
            case HELP:
            case PWD:
                break;
        }
    } while(recv_message->command != EXIT);

    return 0;
}

void start_client(char* ip_addr, int port) {
    int socket_desc;
    int* p_socket = malloc(1);
    struct sockaddr_in server;
    pthread_t receive_thread;

    setlocale(LC_ALL, "");

    server.sin_addr.s_addr = inet_addr(ip_addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printw("Failed to create socket");
        return;
    }

    if (connect(socket_desc, (struct sockaddr*) &server, sizeof(server)) < 0) {
        perror("Connection error");
        return;
    }

    hello_message(socket_desc);

    *p_socket = socket_desc;

    if (pthread_create(&receive_thread, NULL, receive_message_handler, (void*)p_socket) != 0) {
        perror("Could not create a receive thread.");
        return;
    }

    if (current_path[0] != '/') return;

    initscr();
    noecho();
    print_menu();

    send_ls_message(socket_desc, current_path);

    start_screen_handler(socket_desc);

    endwin();
}
