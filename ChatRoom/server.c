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
    int fd;   // write fd for this client
} Client;

// Array for connected clients
Client clients[MAX_CLIENTS];
int client_count = 0;

int server_fd = -1;


//  Prototype for functions
void register_client(const char *fifo_name);
void broadcast_message(const char *sender_fifo, const char *message);
void handle_message(const char *msg);

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
        int bytes = read(server_fd, buffer, sizeof(buffer)-1);
        buffer[bytes] = '\0';

        if (strncmp(buffer, "MSG:", 4) == 0) {
            handle_message(buffer);
        } else {
            register_client(buffer);
        }
    }
}


void register_client(const char *fifo_name) {

    if (client_count >= MAX_CLIENTS) {
        printf("Max clients reached.\n");
        return;
    }

    // Copy FIFO name
    strcpy(clients[client_count].fifo_name, fifo_name);

    // open client's FIFO for writing
    int fd = open(fifo_name, O_WRONLY);
    // comment: open client's FIFO to send welcome message

    if (fd == -1) {
        perror("open client fifo");
        return;  
    }

    clients[client_count].fd = fd;

    // Send WELCOME message to the client
    const char *msg = "WELCOME\n";
    write(fd, msg, strlen(msg));
    // comment: send initial welcome message to client

    printf("Client registered: %s\n", fifo_name);

    client_count++;
}

void handle_message(const char *msg) {

    // Format: MSG:<fifo>:<text>

    const char *p = msg + 4;  // skip "MSG:"
    const char *colon = strchr(p, ':');
    if (!colon) return;

    char sender_fifo[256];
    strncpy(sender_fifo, p, colon - p);
    sender_fifo[colon - p] = '\0';

    const char *text = colon + 1;

    // Build message to broadcast
    char out[512];
    snprintf(out, sizeof(out), "[%s] %s", sender_fifo, text);

    broadcast_message(sender_fifo, out);
}


void broadcast_message(const char *sender_fifo, const char *message) {

    for (int i = 0; i < client_count; i++) {

        // skip sender
        if (strcmp(clients[i].fifo_name, sender_fifo) == 0)
            continue;

        write(clients[i].fd, message, strlen(message));
    }
}



//  Definition: cleanup on exit
void cleanup(int sig) {
    if (server_fd != -1) 
        close(server_fd);

    unlink(SERVER_FIFO);

    printf("\nServer closed.\n");
    exit(0);
}
