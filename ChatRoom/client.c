#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <uuid/uuid.h>

#define SLEEP_TIME 100000

char client_fifo[128];
char server_fifo[128];
char username[50];
char room[100];
char uuid_str[40];


void *reader(void *arg);


int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: ./client <username> <room>\n");
        return 1;
    }

    strcpy(username, argv[1]);
    strcpy(room, argv[2]);

    uuid_t u;
    uuid_generate(u);
    uuid_unparse(u, uuid_str);

    char main_fifo[128];
    snprintf(main_fifo, sizeof(main_fifo), "%s/main_fifo", room);

    char reg_msg[128];
    snprintf(reg_msg, sizeof(reg_msg), "%s:%s", username, uuid_str);

    int main_fd = open(main_fifo, O_WRONLY);
    if (main_fd == -1) {
        printf("Cannot connect to server.\n");
        return 1;
    }
    write(main_fd, reg_msg, strlen(reg_msg));
    close(main_fd);

    snprintf(client_fifo, sizeof(client_fifo), "%s/%s_client_fifo", room, uuid_str);
    snprintf(server_fifo, sizeof(server_fifo), "%s/%s_server_fifo", room, uuid_str);

    while (access(server_fifo, F_OK) != 0) usleep(SLEEP_TIME);

    pthread_t t;
    pthread_create(&t, NULL, reader, NULL);
    pthread_detach(t);

    char msg[256];
    while (1) {
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0';

        int fd = open(client_fifo, O_WRONLY);
        if (fd != -1) {
            write(fd, msg, strlen(msg));
            close(fd);
        }

        if (strcmp(msg, "close") == 0)
            break;
    }

    return 0;
}

void *reader(void *arg)
{
    char buffer[300];

    while (1) {
        int fd = open(server_fifo, O_RDONLY);
        if (fd == -1) { usleep(SLEEP_TIME); continue; }

        int n = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);

        if (n > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }

        usleep(SLEEP_TIME);
    }
}
