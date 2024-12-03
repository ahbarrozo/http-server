#include "server.h"
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    server_config config = {
        .port = 8080,
        .max_connections = SOMAXCONN, // max connections per socket for the machine
        .address = NULL
    };
    
    int server_fd = initialize_server(config);
    if (server_fd < 0) {
        return 1;
    }

    while(1) {
        int* client_fd = malloc(sizeof(int));
        *client_fd = accept_connection(server_fd);
        
        if (*client_fd < 0) {
            free(client_fd);
            continue;  // Handle error and continue accepting
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_fd) != 0) {
            close(*client_fd);
            free(client_fd);
            continue;
        }
    }
}
