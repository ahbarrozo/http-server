#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stddef.h>

/**
 * Server configuration structure
 */
typedef struct {
    int port;
    int max_connections;
    char* address;
} server_config;

/**
 * Initialize an HTTP server with the given configuration
 * @param server_config config : struct containing the port 
 *        number to listen on (int), the maximum number of 
 *        connections (int), and the IPv6 address to bind 
 *        to (char*)
 * @return int : Socket file descriptor on success, -1 on error
 */
int initialize_server(server_config config);

/**
 * Accept a new client connection
 * @param int socket_file_descriptor : Server socket to accept connections on
 * @return int : Client socket file descriptor on success, -1 on error
 */
int accept_connection(int socket_file_descriptor);

/** 
 * Create a thread for a connected client, parsing the HTTP method and path 
 * from the request, handling the URLs and returning appropriate HTTP status 
 * codes with appropriate HTML content (static files).
 * @param void* arg : file descriptor in void pointer format, necessary for 
 *        usage with pthread
 * @return void* : NULL pointer used by pthread (NEED TO UNDERSTAND THIS)
 */
void* handle_client(void* arg);

/**
 * Helper function to read a file into a buffer
 * @param char* filename : file name
 * @param size_t* file_size : file size
 * @return char* : buffer with file content (NULL if file doesn't exist)
 */
char* read_file(const char* filename, size_t* file_size);

#endif // HTTP_SERVER_H
