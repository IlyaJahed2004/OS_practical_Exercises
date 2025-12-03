#include<stdio.h>
#include<dirent.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include <unistd.h>
#include <time.h>

#define CAR_COUNT_PER_SPAWNER 3
#define MAX_SLEEP 3

enum Direction{
    NORTH,
    SOUTH,
    EAST,
    WEST
};

const char* dir_to_string(int dir){
    switch(dir){
        case NORTH: return "NORTH";
        case SOUTH: return "SOUTH";
        case EAST:  return "EAST";
        case WEST:  return "WEST";
        default:    return "UNKNOWN";
    }
}

pthread_mutex_t sections[4];

// helper functions for lock and unlock

void lock_quads(int q[], int n){
    // sort the sections for lock:
    for(int i = 0; i < n - 1; i++){
        for(int j = i + 1; j < n; j++){
            if(q[j] < q[i]){
                int tmp = q[i];
                q[i] = q[j];
                q[j] = tmp;
            }
        }
    }

    // now locking the sections in the sorted order:
    for(int i = 0; i < n; i++){
        pthread_mutex_lock(&sections[q[i]]);
    }
}


void unlock_quads(int q[], int n){
    for(int i = 0; i < n; i++)
        pthread_mutex_unlock(&sections[q[i]]); 
}



void turn_right(int id, int dir){
    int section_involved;

    switch(dir){
        case NORTH: 
            section_involved = 0;
            break;   
        case EAST:  
            section_involved = 1; 
            break;  
        case SOUTH: 
            section_involved = 2; 
            break;  
        case WEST:  
            section_involved = 3; 
            break;   
    }

    pthread_mutex_lock(&sections[section_involved]);

    printf("[ACTION] Car %d from %s turning RIGHT (section %d)\n",
        id,
        dir_to_string(dir),
        section_involved);

    sleep(rand() % MAX_SLEEP);

    pthread_mutex_unlock(&sections[section_involved]);
}


void go_straight(int id, int dir){
    int sections_involoved[2];

    switch(dir){
        case NORTH:
            sections_involoved[0] = 0;
            sections_involoved[1] = 3;
            break;
        case EAST:
            sections_involoved[0] = 3;
            sections_involoved[1] = 2;
            break;
        case SOUTH:
            sections_involoved[0] = 2; 
            sections_involoved[1] = 1;
            break;
        case WEST:
            sections_involoved[0] = 1; 
            sections_involoved[1] = 0;
            break;
    }

    lock_quads(sections_involoved, 2);

    printf("[ACTION] Car %d from %s going STRAIGHT (sections %d -> %d)\n",
        id,
        dir_to_string(dir),
        sections_involoved[0],
        sections_involoved[1]);
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

    printf("[ACTION] Car %d from %s turning LEFT (sections %d -> %d -> %d)\n",
        id,
        dir_to_string(dir),
        sections_involoved[0],
        sections_involoved[1],
        sections_involoved[2]);
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

        int global_id = source * CAR_COUNT_PER_SPAWNER + i;
        const char* move_name;
        switch (move) {
            case 0: move_name = "RIGHT";    break;
            case 1: move_name = "STRAIGHT"; break;
            case 2: move_name = "LEFT";     break;
        }

        printf("[INTENT] Car %d from %s wants to go %s\n",
            global_id,
            dir_to_string(source),
            move_name);
        switch(move){
            case 0: turn_right(global_id, source);  break;
            case 1: go_straight(global_id, source); break;
            case 2: turn_left(global_id, source);   break;
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
