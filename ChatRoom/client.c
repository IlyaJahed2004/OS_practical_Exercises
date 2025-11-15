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

char my_fifo[256];
int server_fd = -1;
int client_read_fd = -1;
volatile sig_atomic_t running = 1;

// Prototype
void cleanup(void);

// Reader thread: continuously read messages from client's FIFO and print
void *reader_thread(void *arg) {
    (void)arg;
    char buf[1024];
    while (running) {
        ssize_t n = read(client_read_fd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s", buf);           // print message from server
            fflush(stdout);
        } else if (n == 0) {
            // EOF: no writers currently; small sleep to avoid busy loop
            usleep(100000);
        } else {
            if (errno == EINTR) continue;
            perror("read client fifo");
            break;
        }
    }
    return NULL;
}

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

// Cleanup close fds and remove fifo
void cleanup(void) {
    if (client_read_fd != -1) close(client_read_fd); // close read end
    if (server_fd != -1) close(server_fd);
    unlink(my_fifo); // remove FIFO file
}

int main(void) {
    signal(SIGINT, handle_sigint);

    // 1) make unique fifo name using PID
    snprintf(my_fifo, sizeof(my_fifo), "/tmp/client_%d_fifo", (int)getpid());

    // 2) create the client FIFO
    if (mkfifo(my_fifo, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo client");
            return 1;
        }
    }
    printf("Created client FIFO: %s\n", my_fifo);

    // 3) open server FIFO for writing and send our FIFO name (register)
    server_fd = open(SERVER_FIFO, O_WRONLY);
    if (server_fd == -1) {
        perror("open server fifo");
        unlink(my_fifo);
        return 1;
    }

    // send fifo name (registration)
    ssize_t w = write(server_fd, my_fifo, strlen(my_fifo));
    if (w == -1) {
        perror("write server fifo");
        cleanup();
        return 1;
    }
    // keep server_fd open for future messages (optional)
    printf("Sent registration to server.\n");

    // 4) open our own fifo for reading (blocking) and spawn reader thread
    client_read_fd = open(my_fifo, O_RDONLY);
    if (client_read_fd == -1) {
        perror("open client fifo (read)");
        cleanup();
        return 1;
    }
    // comment: start a thread to continuously read incoming messages
    pthread_t tid;
    if (pthread_create(&tid, NULL, reader_thread, NULL) != 0) {
        perror("pthread_create");
        cleanup();
        return 1;
    }

    // 5) main thread can be used for user input later.
    // For now, wait until Ctrl+C (SIGINT) to exit.
    printf("Client listener running. Press Ctrl+C to quit.\n");
    while (running) {
        sleep(1);
    }

    // teardown
    running = 0;
    pthread_join(tid, NULL);
    cleanup();
    printf("\nClient exiting.\n");
    return 0;
}
