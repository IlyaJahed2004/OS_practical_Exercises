#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define Buffer_size 10
#define Consumer_count 3
#define Producer_count 2

sem_t fullbuffer;
sem_t emptybuffer;
pthread_mutex_t mutex;

int consume_elementpointer = 0;
int produce_elementpointer = 0;
int buffer[Buffer_size] = {0};

void* consume(void* args){
    while (1) {
        sleep(rand() % 3 + 1); 

        printf("Consumer waiting for fullbuffer...\n");
        sem_wait(&fullbuffer); 
        printf("Consumer passed fullbuffer!\n");

        pthread_mutex_lock(&mutex);
        int buffer_element = buffer[consume_elementpointer];
        printf("Consumer consumed %d from index %d\n", buffer_element, consume_elementpointer);
        buffer[consume_elementpointer] = 0; 
        consume_elementpointer = (consume_elementpointer + 1) % Buffer_size;
        pthread_mutex_unlock(&mutex);

        printf("Consumer releasing emptybuffer...\n");
        sem_post(&emptybuffer); 
        printf("Consumer released emptybuffer!\n");
    }
    return NULL;
}


void* produce(void* args){
    while (1) {
        sleep(rand() % 3 + 1); 

        printf("Producer waiting for emptybuffer...\n");
        sem_wait(&emptybuffer); 
        printf("Producer passed emptybuffer!\n");

        pthread_mutex_lock(&mutex);
        int x = rand() % 100 + 1;
        buffer[produce_elementpointer] = x;
        printf("Producer produced %d at index %d\n", x, produce_elementpointer);
        produce_elementpointer = (produce_elementpointer + 1) % Buffer_size;
        pthread_mutex_unlock(&mutex);

        printf("Producer releasing fullbuffer...\n");
        sem_post(&fullbuffer); 
        printf("Producer released fullbuffer!\n");
    }
    return NULL;
}


int main(){
    srand(time(NULL));

    sem_init(&fullbuffer, 0, 0);              
    sem_init(&emptybuffer, 0, Buffer_size);   
    pthread_mutex_init(&mutex, NULL);

    pthread_t consumer[Consumer_count];
    pthread_t producer[Producer_count];

    for (int i = 0; i < Consumer_count; i++) {
        pthread_create(&consumer[i], NULL, consume, NULL);
    }

    for (int i = 0; i < Producer_count; i++) {
        pthread_create(&producer[i], NULL, produce, NULL);
    }

    for (int i = 0; i < Consumer_count; i++) {
        pthread_join(consumer[i], NULL);
    }

    for (int i = 0; i < Producer_count; i++) {
        pthread_join(producer[i], NULL);
    }

    sem_destroy(&fullbuffer);
    sem_destroy(&emptybuffer);
    pthread_mutex_destroy(&mutex);

    return 0;
}
