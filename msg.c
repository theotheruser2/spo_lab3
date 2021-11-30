#include "msg.h"

void send_constructed_msg(struct constructed_message* constructed_message, int socket_desc) {
    if (write(socket_desc, constructed_message, sizeof(struct constructed_message)) < 0) {
        return;
    }
    if (write(socket_desc, constructed_message->data, constructed_message->data_length * sizeof(char)) < 0) {
        return;
    }
}

struct constructed_message* receive_constructed_msg(int socket_desc) {
    struct constructed_message* receiving_message = calloc(1, sizeof(struct constructed_message));
    read(socket_desc, receiving_message, sizeof(struct constructed_message));
    receiving_message->data = malloc(receiving_message->data_length * sizeof(char));
    read(socket_desc, receiving_message->data, receiving_message->data_length);
    return receiving_message;
}

struct constructed_message* copy_constructed_msg(struct constructed_message* src) {
    struct constructed_message* dst = NULL;
    dst = calloc(1, sizeof(struct constructed_message));
    dst->data = calloc(1, src->data_length);
    memcpy(dst, src, sizeof(struct constructed_message) + src->data_length);
    return dst;
}

void free_constructed_msg(struct constructed_message* message) {
    if (message == NULL) return;
    if (message->data != NULL) {
        free(message->data);
    }
    free(message);
}
