/*
 * A simple HTTP server
 * ---
 *
 *  initialize_server()
 *  1- An endpoint for communication is created using socket(), returning a file descriptor index
 *  2- The socket is bound to an address (a.k.a assigning a name to the socket). We tell which 
 *     socket type, the port number (conversion of value to network byte order). A casting of 
 *     variable type was necessary as bind() accepts normally IPv4 sockaddr_in structs.
 *  3- The server starts listening for new connections
 *
 *  accept_connection()
 *  4- Upon accepting a connection, we retrieve information from the client
 *
 **/

#include "server.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int initialize_server(server_config config) {

    struct sockaddr_in6 socket_addr;
    // Create the socket
    int socket_file_descriptor = socket(AF_INET6, SOCK_STREAM, 0);

    if (socket_file_descriptor < 0) {
        perror("Unable to initialize socket");
        exit(EXIT_FAILURE);
    }

    socket_addr.sin6_family = AF_INET6;

    if (config.address == NULL) {
        socket_addr.sin6_addr = in6addr_any;
    } else {
        int assign_address = inet_pton(AF_INET6, config.address, (void *)&socket_addr.sin6_addr.s6_addr);

        if (assign_address < 0) {
            perror("Invalid address format");
            exit(EXIT_FAILURE);
        }
    }
    socket_addr.sin6_port = htons(config.port);

    socket_addr.sin6_scope_id = 0;

    int bind_response = bind(socket_file_descriptor, (struct sockaddr*)&socket_addr, sizeof(socket_addr));

    if (bind_response < 0) {
        perror("Unable to bind socket");
        exit(EXIT_FAILURE);
    }

    int listen_response = listen(socket_file_descriptor, config.max_connections);

    if (listen_response < 0) {
        perror("Connection refused");
        close(socket_file_descriptor);
        return -1;
    }

    return socket_file_descriptor;
}

int accept_connection(int socket_file_descriptor) {

    struct sockaddr_in6 client_addr;

    // Tracking the client requesting for the connection
    socklen_t client_addr_len = sizeof(client_addr);
    int client_file_descriptor = accept(socket_file_descriptor, (struct sockaddr*)&client_addr, &client_addr_len);

    if (client_file_descriptor < 0) {
        perror("Accept failed");
        return -1;
    }

    return client_file_descriptor;
}

char* read_file(const char* filename, size_t* file_size) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return NULL;
    }

    // Allocate buffer for file content
    char* buffer = malloc(st.st_size + 1);
    if (!buffer) {
        close(fd);
        return NULL;
    }

    // Read file content
    ssize_t bytes_read = read(fd, buffer, st.st_size);
    close(fd);

    if (bytes_read < 0) {
        free(buffer);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    *file_size = bytes_read;
    return buffer;
}

void* handle_client(void* arg) {
    int client_fd = *((int*)arg);
    int http_code;

    free(arg);
    pthread_detach(pthread_self());

    char buffer[4096]; // 4 kb should suffice for HTTP headers
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0) {
        perror("Error reading from client");
        close(client_fd);
        return NULL;
    }

    buffer[bytes_read] = '\0';

    // Parse the request method and path
    char method[16];
    char path[256];
    char header[512];
    sscanf(buffer, "%s %s", method, path);

    printf("Method: %s, Path: %s\n", method, path);

    if (strcmp(method, "GET") == 0) {
        const char* filename;
        if (strcmp(path, "/") == 0) {
            filename = "index.html";
            http_code = 200;
        } else if (strcmp(path, "/about") == 0) {
            filename = "about.html";
            http_code = 200;
        } else {
            filename = "404.html";
            http_code = 404;
        }

        // Try to read the file
        size_t file_size;
        char* file_content = read_file(filename, &file_size);
        
        if (file_content) {
            // Create and send HTTP response header
            if (http_code == 404) {
                sprintf(header, 
                        "HTTP/1.1 404 Not Found\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n", file_size);
            } else {
                sprintf(header, 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: %zu\r\n"
                    "Connection: close\r\n"
                    "\r\n", file_size);
            }
            
            // Send header and file content
            write(client_fd, header, strlen(header));
            write(client_fd, file_content, file_size);
            
            free(file_content);
        } else {
            // 500 Internal Server Error
            const char* error = 
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<html><body><h1>500 Internal Server Error</h1></body></html>";
            write(client_fd, error, strlen(error));
        }
    }

    close(client_fd);
    return NULL;
}
