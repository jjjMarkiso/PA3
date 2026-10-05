#include "server.h"

// Mutex used to protect control-message output.
pthread_mutex_t mutex_client_socket =
    PTHREAD_MUTEX_INITIALIZER;

// Print control messages safely.
void print_control_message(const char *message)
{
    pthread_mutex_lock(&mutex_client_socket);
    printf("%s\n", message);
    fflush(stdout);
    pthread_mutex_unlock(&mutex_client_socket);
}


// Handle one client.
void *handle_client(void *arg)
{
    // Make a local copy of the client's socket.
    int client_socket = *((int *)arg);

    // The dynamically allocated socket descriptor is no longer needed.
    free(arg);

    int nist_socket;

    struct addrinfo hints;
    struct addrinfo *server_info;
    struct addrinfo *p;

    char buffer[BUFFER_SIZE];

    int bytes_received;
    int bytes_sent;

    // Client connected
    print_control_message(
        "Client connected."
    );

    // Find NIST server
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int result = getaddrinfo(NIST_SERVER, NIST_PORT, &hints, &server_info);

    if (result != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        close(client_socket);
        pthread_exit(NULL);
    }

    // Connect to NIST
    nist_socket = -1;

    for (p = server_info; p != NULL; p = p->ai_next)
    {
        nist_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (nist_socket == -1)
        {
            continue;
        }

        if (connect( nist_socket, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }

        close(nist_socket);
        nist_socket = -1;
    }

    freeaddrinfo(server_info);

    if (nist_socket == -1)
    {
        print_control_message("Could not connect to NIST.");
        close(client_socket);

        pthread_exit(NULL);
    }

    print_control_message("Connected to NIST Daytime server.");

    // Receive NIST timestamp
    bytes_received = recv(nist_socket, buffer, sizeof(buffer), 0);

    if (bytes_received < 0)
    {
        perror("Error receiving from NIST");

        close(nist_socket);
        close(client_socket);

        pthread_exit(NULL);
    }

    // Forward timestamp to client
    bytes_sent = send(client_socket, buffer, bytes_received, 0);

    if (bytes_sent < 0)
    {
        perror("Error sending to client");
    }
    else
    {
        print_control_message("NIST timestamp forwarded to client.");
    }

    // Cleanup
    close(nist_socket);
    print_control_message("Closed connection to NIST.");

    close(client_socket);
    print_control_message("Closed connection to client.");

    pthread_exit(NULL);
}


// Main server
int main(void)
{
    int server_socket;
    struct sockaddr_in server_address;
    int yes = 1;

    signal(SIGPIPE, SIG_IGN);

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Allow port reuse.
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) != 0)
    {
        perror("Error binding socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen

    if (listen(server_socket, NUM_CONNECTIONS) != 0)
    {
        perror("Error listening on socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Daytime Proxy Server listening on port %d\n", PORT);
    printf("NIST server: %s:%s\n", NIST_SERVER, NIST_PORT);

    // Server loop
    while (TRUE)
    {
        // Allocate a separate socket descriptor for each thread.
        int *client_socket = malloc(sizeof(int));

        if (client_socket == NULL)
        {
            perror("malloc");
            continue;
        }

        // Accept client.
        *client_socket = accept(server_socket, NULL, NULL);

        if (*client_socket == -1)
        {
            perror("Error accepting client");
            free(client_socket);
            continue;
        }

        // Create client thread.
        pthread_t thread;

        if (pthread_create(&thread, NULL, handle_client, client_socket) != 0)
        {
            perror("Error creating thread");
            close(*client_socket);
            free(client_socket);
            continue;
        }

        // Detach thread
        if (pthread_detach(thread) != 0)
        {
            perror("Error detaching thread");
        }
    }

    close(server_socket);

    return 0;
}