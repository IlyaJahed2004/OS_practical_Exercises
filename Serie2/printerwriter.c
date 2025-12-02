
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#define NUM_WRITERS 20

// Shared resources here (TODO)
sem_t printers;
pthread_mutex_t mutex;

void* writer(void* arg)
{
    int id = *((int *)arg);
    // TODO: wait for access to preparation area
    pthread_mutex_lock(&mutex);

    printf("write %d started preparing\n", id);
    sleep(rand()%3 + 1);
    printf("write %d finished preparing\n", id);
    
    // TODO: release preparation area
    pthread_mutex_unlock(&mutex);

    // TODO: wait for access to printer
    sem_wait(&printers);
    printf("write %d started printing\n", id);
    sleep(rand()%3 + 2);
    printf("write %d finished printing\n", id);

    // TODO: release printer
    sem_post(&printers);

    printf("writer %d exited the system\n", id);
   

    return NULL;
}

int main()
{
    // TODO: initialize shared resources
    sem_init(&printers, 0,2);
    pthread_mutex_init(&mutex, NULL);
    pthread_t writers[NUM_WRITERS];
    int ids[NUM_WRITERS];



    for (int i = 0; i < NUM_WRITERS; i++)
    {
        ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer, &ids[i]);
    }

    for (int i = 0; i < NUM_WRITERS; i++)
        pthread_join(writers[i], NULL);



    // TODO: cleanup shared resources
    sem_destroy(&printers);
    pthread_mutex_destroy(&mutex);

    return 0;
}
