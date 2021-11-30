#include "opts.h"
#include "client.h"
#include "server.h"

/*
 * commands - PWD, CD, CAT, UPLOAD, DOWNLOAD, HELP, QUIT
 */

int main(int argc, char** argv) {
    struct options options;
    get_options(argc, argv, &options);
    if (options.mode == CLIENT) {
        start_client(options.client_params.server_ip, options.server_port);
    } else if (options.mode == SERVER) {
        start_server(options.server_params.root_path, options.server_port);
    }
    return 0;
}
