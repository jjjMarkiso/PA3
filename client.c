#include "client.h"

int main(void)
{
    int client_socket;

    struct addrinfo hints;
    struct addrinfo *server_info;
    struct addrinfo *p;

    char buffer[BUFFER_SIZE];

    int bytes_received;
    int result;

    // Configure getaddrinfo()
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Find the proxy server.
    result = getaddrinfo(SERVER_ADDR, SERVER_PORT, &hints, &server_info);

    if (result != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));

        exit(EXIT_FAILURE);
    }

    // Connect to proxy
    client_socket = -1;

    for (p = server_info; p != NULL; p = p->ai_next)
    {
        client_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (client_socket == -1)
        {
            continue;
        }

        if (connect(client_socket, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }

        close(client_socket);
        client_socket = -1;
    }

    freeaddrinfo(server_info);

    if (client_socket == -1)
    {
        perror("Could not connect to proxy");
        exit(EXIT_FAILURE);
    }

    printf("Connected to Daytime proxy.\n");

    // Receive timestamp
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received < 0)
    {
        perror("Error receiving data");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Add null terminator so we can safely print the received data as a string.
    buffer[bytes_received] = '\0';

    printf("\nResponse from proxy:\n");
    printf("%s\n", buffer);

    // Cleanup
    close(client_socket);

    printf("Connection closed.\n");

    return 0;
}