// server.c
// gcc -o server server.c
// Simple FIFO chat server (named pipe). Keeps list of clients and broadcasts messages.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define SERVER_FIFO "/tmp/server_fifo"
#define MAX_CLIENTS 10

// Structure to store each client's FIFO name and write-FD
typedef struct {
    char fifo_name[256];
    int fd;   // write fd for this client
} Client;

// global client array and counters
static Client clients[MAX_CLIENTS];
static int client_count = 0;

static int server_fd = -1;
static int dummy_fd = -1; // keep write-end open to avoid EOF on read

// prototypes
void register_client(const char *fifo_name);
void broadcast_message(const char *sender_fifo, const char *message);
void handle_message(const char *msg);
void remove_client(int index);
void cleanup(int sig);
int find_client_index(const char *fifo_name);

int main(void) {
    // handle signals for clean exit
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    // remove old server FIFO if exists
    unlink(SERVER_FIFO);

    // create server FIFO
    if (mkfifo(SERVER_FIFO, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            return 1;
        }
    }

    printf("Server FIFO created: %s\n", SERVER_FIFO);

    // open server FIFO for reading
    server_fd = open(SERVER_FIFO, O_RDONLY);
    if (server_fd == -1) {
        perror("open server fifo (read)");
        cleanup(0);
    }

    // open a dummy write end so read() doesn't get EOF when no clients are connected
    dummy_fd = open(SERVER_FIFO, O_WRONLY | O_NONBLOCK);
    // it's okay if dummy_fd fails (no writer yet), we continue

    printf("Server is ready and waiting...\n");

    char buffer[1024];

    while (1) {
        ssize_t bytes = read(server_fd, buffer, sizeof(buffer) - 1);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            // There may be multiple messages in buffer separated by newlines.
            // Split by newline and handle each line.
            char *saveptr = NULL;
            char *line = strtok_r(buffer, "\n", &saveptr);
            while (line) {
                // ignore empty lines
                if (line[0] != '\0') {
                    if (strncmp(line, "MSG:", 4) == 0) {
                        handle_message(line);
                    } else if (strncmp(line, "QUIT:", 5) == 0) {
                        // client asks to quit: format QUIT:/tmp/client_x_fifo
                        const char *fifo = line + 5;
                        int idx = find_client_index(fifo);
                        if (idx != -1) remove_client(idx);
                    } else {
                        // treat as registration (fifo name)
                        register_client(line);
                    }
                }
                line = strtok_r(NULL, "\n", &saveptr);
            }
        } else if (bytes == 0) {
            // read returned EOF (all writers closed). Reopen server FIFO for read.
            close(server_fd);
            server_fd = open(SERVER_FIFO, O_RDONLY);
            if (server_fd == -1) {
                perror("reopen server fifo");
                break;
            }
            // re-open dummy write (optional)
            if (dummy_fd == -1) {
                dummy_fd = open(SERVER_FIFO, O_WRONLY | O_NONBLOCK);
            }
        } else {
            if (errno == EINTR) continue;
            perror("read server fifo");
            break;
        }
    }

    cleanup(0);
    return 0;
}


// find client index by fifo name, -1 if not found
int find_client_index(const char *fifo_name) {
    for (int i = 0; i < client_count; ++i) {
        if (strcmp(clients[i].fifo_name, fifo_name) == 0) return i;
    }
    return -1;
}


// Register a new client: open its FIFO for writing and store fd
void register_client(const char *fifo_name) {
    // trim potential whitespace
    char name[256];
    strncpy(name, fifo_name, sizeof(name)-1);
    name[sizeof(name)-1] = '\0';

    if (find_client_index(name) != -1) {
        printf("Client already registered: %s\n", name);
        return;
    }

    if (client_count >= MAX_CLIENTS) {
        printf("Max clients reached. Rejecting: %s\n", name);
        return;
    }

    // copy name
    strncpy(clients[client_count].fifo_name, name, sizeof(clients[client_count].fifo_name)-1);
    clients[client_count].fifo_name[sizeof(clients[client_count].fifo_name)-1] = '\0';

    // open client's FIFO for writing (blocking); this waits until client opens read-end
    int fd = open(name, O_WRONLY);
    if (fd == -1) {
        perror("open client fifo (write)");
        // do not register if cannot open
        return;
    }

    clients[client_count].fd = fd;

    // send welcome message
    const char *msg = "WELCOME\n";
    if (write(fd, msg, strlen(msg)) == -1) {
        perror("write welcome");
        // if write failed, remove client immediately
        close(fd);
        return;
    }

    printf("Client registered: %s (fd=%d)\n", name, fd);
    // broadcast join message to others
    char joinmsg[512];
    snprintf(joinmsg, sizeof(joinmsg), "*** %s joined the chat\n", name);
    broadcast_message(name, joinmsg);

    client_count++;
}


// Remove client at index, closing fd and shifting array
void remove_client(int index) {
    if (index < 0 || index >= client_count) return;

    printf("Removing client: %s\n", clients[index].fifo_name);
    // broadcast leave message
    char leavemsg[512];
    snprintf(leavemsg, sizeof(leavemsg), "*** %s left the chat\n", clients[index].fifo_name);
    broadcast_message(clients[index].fifo_name, leavemsg);

    // close write fd
    if (clients[index].fd != -1) close(clients[index].fd);

    // shift left
    for (int i = index; i < client_count - 1; ++i) {
        clients[i] = clients[i+1];
    }
    client_count--;
}


// messages come as "MSG:<fifo>:<text>"
void handle_message(const char *msg) {
    const char *p = msg + 4;  // skip "MSG:"
    const char *colon = strchr(p, ':');
    if (!colon) return;

    char sender_fifo[256];
    size_t n = colon - p;
    if (n >= sizeof(sender_fifo)) n = sizeof(sender_fifo)-1;
    strncpy(sender_fifo, p, n);
    sender_fifo[n] = '\0';

    const char *text = colon + 1;

    // Build message to broadcast
    char out[1024];
    // Keep the newline from client text if present
    snprintf(out, sizeof(out), "[%s] %s", sender_fifo, text);

    broadcast_message(sender_fifo, out);
}


// Broadcast message to all clients except sender
void broadcast_message(const char *sender_fifo, const char *message) {
    for (int i = 0; i < client_count; ++i) {
        if (strcmp(clients[i].fifo_name, sender_fifo) == 0) continue;
        ssize_t w = write(clients[i].fd, message, strlen(message));
        if (w == -1) {
            if (errno == EPIPE || errno == EBADF) {
                // client likely disconnected; remove it
                remove_client(i);
                i--; // adjust loop after shift
            } else {
                perror("write to client");
            }
        }
    }
}


// cleanup: close all client fds and remove server fifo
void cleanup(int sig) {
    (void)sig;
    printf("\nCleaning up server...\n");
    if (server_fd != -1) close(server_fd);
    if (dummy_fd != -1) close(dummy_fd);

    // close client fds
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].fd != -1) close(clients[i].fd);
    }

    unlink(SERVER_FIFO);
    printf("Server closed.\n");
    exit(0);
}
