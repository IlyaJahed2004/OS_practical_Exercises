#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>   // for strcpy()

#define SERVER_FIFO "/tmp/server_fifo"
#define MAX_CLIENTS 10


// Structure to store each client's FIFO name
typedef struct {
    char fifo_name[256];
} Client;

// Array for connected clients
Client clients[MAX_CLIENTS];
int client_count = 0;

int server_fd = -1;


//  Prototype for functions
void register_client(const char *fifo_name);
void cleanup(int sig);


int main() {

    // Register signal handlers (Ctrl+C, kill, etc.)
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    unlink(SERVER_FIFO); // remove old FIFO if it exists

    // Create FIFO with read/write permissions
    if (mkfifo(SERVER_FIFO, 0666) == -1) {
        perror("mkfifo");
        return 1;
    }

    printf("FIFO created.\n");

    // Open FIFO for reading only
    server_fd = open(SERVER_FIFO, O_RDONLY);
    if (server_fd == -1) {
        perror("open");
        cleanup(0);
    }

    printf("Server is ready and waiting...\n");

    char buffer[256];

    while (1) {
        int bytes = read(server_fd, buffer, sizeof(buffer));

        if (bytes > 0) {
            buffer[bytes] = '\0';   // make it a string
            register_client(buffer); // client's FIFO name
        }
    }
}


//  Definition: register client
void register_client(const char *fifo_name) {
    if (client_count >= MAX_CLIENTS) {
        printf("Max clients reached.\n");
        return;
    }

    strcpy(clients[client_count].fifo_name, fifo_name);
    client_count++;

    printf("Client registered: %s\n", fifo_name);
}


//  Definition: cleanup on exit
void cleanup(int sig) {
    if (server_fd != -1) 
        close(server_fd);

    unlink(SERVER_FIFO);

    printf("\nServer closed.\n");
    exit(0);
}
