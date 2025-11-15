#include <stdio.h>     // printf, perror
#include <stdlib.h>    // exit
#include <unistd.h>    // write, close
#include <fcntl.h>     // open
#include <sys/stat.h>  // mkfifo
#include <string.h>    // strcpy, strlen

#define SERVER_FIFO "/tmp/server_fifo"


int main() {

    // Step 1: make client FIFO name
    char my_fifo[256];
    sprintf(my_fifo, "/tmp/client_%d_fifo", getpid());  
    // comment: create a unique fifo name using PID


    // Step 2: create the client FIFO
    if (mkfifo(my_fifo, 0666) == -1) {
        perror("mkfifo client");
        return 1;
    }
    printf("Client FIFO created: %s\n", my_fifo);


    // Step 3: open server FIFO (write only)
    int server_fd = open(SERVER_FIFO, O_WRONLY);
    // comment: connect to the server by opening its FIFO

    if (server_fd == -1) {
        perror("open server fifo");
        unlink(my_fifo);
        return 1;
    }


    // Step 4: send my FIFO name to server
    write(server_fd, my_fifo, strlen(my_fifo));
    // comment: inform server about my fifo name

    printf("Client registered with server.\n");


    // Step 5: cleanup
    close(server_fd);

    printf("Done. (Later we will read messages here.)\n");

    return 0;
}
