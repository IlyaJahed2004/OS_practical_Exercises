#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>

#define MAX_CLIENTS 10
#define BUF_SIZE 256
#define SLEEP_TIME 100000

typedef struct {
    char name[50];
    char uuid[40];
    char client_fifo[128];
    char server_fifo[128];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

void send_message(const char *sender, const char *uuid, const char *msg);
void send_online(const char *sender, const char *room);
void *client_thread(void *arg);
void *keyboard_thread(void *arg);



int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: ./server <room>\n");
        return 1;
    }

    char *room = argv[1];
    mkdir(room, 0777);

    char main_fifo[128];
    snprintf(main_fifo, sizeof(main_fifo), "%s/main_fifo", room);

    mkfifo(main_fifo, 0666);

    printf("Chatroom '%s' started. Type \"close\" when empty to exit.\n", room);

    pthread_t key;
    pthread_create(&key, NULL, keyboard_thread, NULL);
    pthread_detach(key);

    while (1) {
        int fd = open(main_fifo, O_RDONLY);
        if (fd == -1) continue;

        char buf[200];
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) {
            close(fd);
            continue;
        }
        buf[n] = '\0';
        close(fd);

        char username[50], uuid[40];
        sscanf(buf, "%49[^:]:%39s", username, uuid);

        if (client_count >= MAX_CLIENTS) {
            printf("Room full!\n");
            continue;
        }

        Client *c = &clients[client_count++];
        strcpy(c->name, username);
        strcpy(c->uuid, uuid);

        snprintf(c->client_fifo, sizeof(c->client_fifo), "%s/%s_client_fifo", room, uuid);
        snprintf(c->server_fifo, sizeof(c->server_fifo), "%s/%s_server_fifo", room, uuid);

        mkfifo(c->client_fifo, 0666);
        mkfifo(c->server_fifo, 0666);

        for (int i = 0; i < 50; i++) {
            int test = open(c->server_fifo, O_WRONLY | O_NONBLOCK);
            if (test != -1) { close(test); break; }
            usleep(SLEEP_TIME);
        }

        send_online(c->name, room);

        time_t now = time(NULL);
        printf("[%s] Client '%s' joined.\n", strtok(ctime(&now), "\n"), username);

        pthread_t t;
        pthread_create(&t, NULL, client_thread, c);
        pthread_detach(t);
    }

    return 0;
}



void send_online(const char *sender, const char *room)
{
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].name, sender) == 0) {
            int fd = open(clients[i].server_fifo, O_WRONLY);
            dprintf(fd, "[SYSTEM] Welcome %s to chat room %s! online users: %d\n",
                    sender, room, client_count - 1);
            close(fd);
            return;
        }
    }
}

void send_message(const char *sender, const char *uuid, const char *msg)
{
    for (int i = 0; i < client_count; i++) {
        int fd = open(clients[i].server_fifo, O_WRONLY | O_NONBLOCK);
        if (fd == -1) continue;

        if (strcmp(clients[i].uuid, uuid) == 0)
            dprintf(fd, "You: %s\n", msg);
        else
            dprintf(fd, "%s: %s\n", sender, msg);

        close(fd);
    }
}

void *client_thread(void *arg)
{
    Client *c = arg;
    char buffer[BUF_SIZE];

    while (1) {
        int fd = open(c->client_fifo, O_RDONLY);
        if (fd == -1) { usleep(SLEEP_TIME); continue; }

        int n = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);

        if (n <= 0) continue;

        buffer[n] = '\0';

        if (strcmp(buffer, "close") == 0) {
            printf("Client '%s' left.\n", c->name);
            return NULL;
        }

        send_message(c->name, c->uuid, buffer);
    }
}

void *keyboard_thread(void *arg)
{
    char msg[50];
    while (1) {
        fgets(msg, sizeof(msg), stdin);

        if (strcmp(msg, "close\n") == 0 && client_count == 0) {
            printf("Chat closed.\n");
            exit(0);
        } else {
            printf("Cannot close while clients are connected.\n");
        }
    }
}
