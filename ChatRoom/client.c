// client.c
// gcc -o client client.c -lpthread
// Simple FIFO chat client: register, listen and send messages.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define SERVER_FIFO "/tmp/server_fifo"

static char my_fifo[256];
static int server_fd = -1;       // write fd to server FIFO
static int client_read_fd = -1;  // read fd for my FIFO
volatile sig_atomic_t running = 1;

// prototypes
void cleanup(void);
void handle_sigint(int sig);
void *reader_thread(void *arg);

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

// Reader thread: continuously read messages from client's FIFO and print
void *reader_thread(void *arg) {
    (void)arg;
    char buf[2048];
    while (running) {
        ssize_t n = read(client_read_fd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            // Print incoming message; keep user input UX simple
            printf("%s", buf);
            fflush(stdout);
        } else if (n == 0) {
            // EOF: no writers currently; small sleep to avoid busy loop
            usleep(100000);
        } else {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // nothing to read
                usleep(100000);
                continue;
            }
            perror("read client fifo");
            break;
        }
    }
    return NULL;
}

// Cleanup close fds and remove fifo
void cleanup(void) {
    if (client_read_fd != -1) close(client_read_fd); // close read end
    if (server_fd != -1) close(server_fd);
    unlink(my_fifo); // remove FIFO file (client is owner)
}

int main(void) {
    // handle Ctrl+C
    signal(SIGINT, handle_sigint);

    // 1) make unique fifo name using PID
    snprintf(my_fifo, sizeof(my_fifo), "/tmp/client_%d_fifo", (int)getpid());

    // 2) create the client FIFO (ignore if exists)
    if (mkfifo(my_fifo, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo client");
            return 1;
        }
    }
    printf("Created client FIFO: %s\n", my_fifo);

    // 3) open server FIFO for writing (register)
    server_fd = open(SERVER_FIFO, O_WRONLY);
    if (server_fd == -1) {
        perror("open server fifo");
        unlink(my_fifo);
        return 1;
    }

    // send fifo name (registration) + newline so server can split lines
    {
        char regline[300];
        snprintf(regline, sizeof(regline), "%s\n", my_fifo);
        ssize_t w = write(server_fd, regline, strlen(regline));
        if (w == -1) {
            perror("write server fifo");
            cleanup();
            return 1;
        }
    }
    printf("Sent registration to server.\n");

    // 4) open our own fifo for reading (blocking). Server will open write-end after reading registration.
    client_read_fd = open(my_fifo, O_RDONLY);
    if (client_read_fd == -1) {
        perror("open client fifo (read)");
        cleanup();
        return 1;
    }

    // start a thread to continuously read incoming messages
    pthread_t tid;
    if (pthread_create(&tid, NULL, reader_thread, NULL) != 0) {
        perror("pthread_create");
        cleanup();
        return 1;
    }

    printf("Client listener running. Type messages and press Enter. Type /quit to exit.\n");

    // main thread now reads user input and sends messages
    char msg[1024];
    while (running) {
        if (fgets(msg, sizeof(msg), stdin) == NULL) {
            if (errno == EINTR) continue;
            break;
        }

        // handle quit command
        if (strncmp(msg, "/quit", 5) == 0) {
            // tell server we are quitting: QUIT:<fifo>\n
            char q[300];
            snprintf(q, sizeof(q), "QUIT:%s\n", my_fifo);
            write(server_fd, q, strlen(q));
            running = 0;
            break;
        }

        // send normal message: MSG:<fifo>:<text>
        char buffer[1500];
        snprintf(buffer, sizeof(buffer), "MSG:%s:%s", my_fifo, msg);
        ssize_t w = write(server_fd, buffer, strlen(buffer));
        if (w == -1) {
            if (errno == EPIPE) {
                fprintf(stderr, "Server closed the pipe.\n");
                running = 0;
                break;
            } else {
                perror("write to server");
            }
        }
    }

    // teardown
    running = 0;
    pthread_join(tid, NULL);
    cleanup();
    printf("\nClient exiting.\n");
    return 0;
}
