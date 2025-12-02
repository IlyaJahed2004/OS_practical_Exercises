#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define Philosopher_count 5
#define Fork_count 5

pthread_mutex_t forks[Fork_count];

void* philosopher(void* arg) {
    int id = *((int*)arg);

    // Even philosophers pick left then right
    if (id % 2 == 0) {
        pthread_mutex_lock(&forks[id]);
        printf("Philosopher %d picked left fork %d\n", id, id);

        pthread_mutex_lock(&forks[(id + 4) % 5]);
        printf("Philosopher %d picked right fork %d\n", id, (id + 4) % 5);
    }
    // Odd philosophers pick right then left
    else {
        pthread_mutex_lock(&forks[(id + 4) % 5]);
        printf("Philosopher %d picked right fork %d\n", id, (id + 4) % 5);

        pthread_mutex_lock(&forks[id]);
        printf("Philosopher %d picked left fork %d\n", id, id);
    }

    sleep(1);
    printf("Philosopher %d is eating.\n", id);

    pthread_mutex_unlock(&forks[id]);
    pthread_mutex_unlock(&forks[(id + 4) % 5]);

    printf("Philosopher %d put down both forks number %d and %d and finished eating.\n", id, id,(id+4)%5);

    return NULL;
}

int main() {
    pthread_t th[Philosopher_count];
    int ids[Philosopher_count];

    // Initialize forks
    for (int i = 0; i < Fork_count; i++)
        pthread_mutex_init(&forks[i], NULL);

    // Create philosopher threads
    for (int i = 0; i < Philosopher_count; i++) {
        ids[i] = i;
        printf("Philosopher %d is hungry.\n", i);

        if (pthread_create(&th[i], NULL, philosopher, &ids[i]) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    // Wait for them to finish
    for (int i = 0; i < Philosopher_count; i++) {
        pthread_join(th[i], NULL);
    }

    // Destroy mutexes
    for (int i = 0; i < Fork_count; i++)
        pthread_mutex_destroy(&forks[i]);

    return 0;
}
