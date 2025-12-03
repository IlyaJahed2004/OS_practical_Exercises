#include<stdio.h>
#include<dirent.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include <unistd.h>
#include <time.h>

#define CAR_COUNT_PER_SPAWNER 20
#define MAX_SLEEP 3

enum Direction{
    NORTH,
    SOUTH,
    EAST,
    WEST
};

pthread_mutex_t sections[4];

// helper functions for lock and unlock

void lock_quads(int q[], int n){
    for(int i = 0; i < n; i++)
        pthread_mutex_lock(&sections[q[i]]);
}

void unlock_quads(int q[], int n){
    for(int i = 0; i < n; i++)
        pthread_mutex_unlock(&sections[q[i]]);
}



void turn_right(int id, int dir){
    int section_involved;

    switch(dir){
        case NORTH: section_involved = 0; break;   
        case EAST:  section_involved = 1; break;  
        case SOUTH: section_involved = 2; break;  
        case WEST:  section_involved = 3; break;   
    }

    pthread_mutex_lock(&sections[section_involved]);

    printf("Car %d turning RIGHT from %d\n", id, dir);
    sleep(rand() % MAX_SLEEP);

    pthread_mutex_unlock(&sections[section_involved]);
}


void go_straight(int id, int dir){
    int sections_involoved[2];

    switch(dir){
        case NORTH:
            sections_involoved[0] = 1; sections_involoved[1] = 2;
            break;
        case EAST:
            sections_involoved[0] = 2; sections_involoved[1] = 3;
            break;
        case SOUTH:
            sections_involoved[0] = 3; sections_involoved[1] = 0;
            break;
        case WEST:
            sections_involoved[0] = 0; sections_involoved[1] = 1;
            break;
    }

    lock_quads(sections_involoved, 2);

    printf("Car %d going STRAIGHT from %d\n", id, dir);
    sleep(rand() % MAX_SLEEP);

    unlock_quads(sections_involoved, 2);
}


void turn_left(int id, int dir){
    int sections_involoved[3];

    switch(dir){
        case NORTH:
            sections_involoved[0] = 0;
            sections_involoved[1] = 3;
            sections_involoved[2] = 2;
            break;
        case EAST:
            sections_involoved[0] = 1;
            sections_involoved[1] = 0;
            sections_involoved[2] = 3;
            break;
        case SOUTH:
            sections_involoved[0] = 2;
            sections_involoved[1] = 1;
            sections_involoved[2] = 0;
            break;
        case WEST:
            sections_involoved[0] = 3;
            sections_involoved[1] = 2;
            sections_involoved[2] = 1;
            break;
    }

    lock_quads(sections_involoved, 3);

    printf("Car %d turning LEFT from %d\n", id, dir);
    sleep(rand() % MAX_SLEEP);

    unlock_quads(sections_involoved, 3);
}

typedef struct{
    int source;
    int count;
} SpawnerArgs;

// DO NOT CHANGE
void* car_spawner(void* args){
    SpawnerArgs* spawner_args = (SpawnerArgs*)args;
    int source = spawner_args->source;
    int count = spawner_args->count;

    for(int i = 0; i < count; i++){
        int move = rand() % 3;

        switch(move){
            case 0: turn_right(i, source);  break;
            case 1: go_straight(i, source); break;
            case 2: turn_left(i, source);   break;
        }
    }
}

// DO NOT CHANGE - Simulation
int main (){
    pthread_t spawner_tid[4];
    SpawnerArgs args[4];

    srand(time(NULL));

    for(int i = 0; i < 4; i++)
        pthread_mutex_init(&sections[i], NULL);

    clock_t time = clock();

    for(int i = 0; i < 4; i++){
        args[i].source = i;
        args[i].count = CAR_COUNT_PER_SPAWNER;
        pthread_create(&spawner_tid[i], NULL, car_spawner, (void*)&args[i]);
    }

    for(int i = 0; i < 4; i++)
        pthread_join(spawner_tid[i], NULL);

    time = clock() - time;
    double runtime = ((double)time) / CLOCKS_PER_SEC;
    printf("The simulation took %f seconds\n", runtime);

    return 0;
}
