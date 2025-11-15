#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#define SERVER_FIFO "/tmp/server_fifo"

int server_fd = -1;

// Clean up FIFO on exit
void cleanup(int sig) {
    if (server_fd != -1) 
        close(server_fd); // close FIFO file descriptor

    unlink(SERVER_FIFO);  // remove FIFO file from filesystem

    printf("\nServer closed.\n");
    exit(0);
}

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

    while (1) {
        pause(); // wait forever until signal arrives
    }
}
