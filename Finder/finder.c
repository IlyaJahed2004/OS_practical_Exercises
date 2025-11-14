// Enable POSIX extensions
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <semaphore.h>   // <-- NEW: semaphore

// ------------ GLOBAL OBJECTS ------------

// MAX number of threads allowed to run at the same time
#define MAX_THREADS 32

// Semaphore to limit concurrency
sem_t thread_sem;

// Mutex only for synchronized printing
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;


// Structure passed to each thread
typedef struct {
    char *path;    // directory path this thread will search
    char *target;  // filename we want to find
} thread_arg_t;


// Prototypes
void *thread_func(void *arg);
void search_dir(const char *path, const char *target);



int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <start_directory> <target_filename>\n", argv[0]);
        return 1;
    }

    char *start = argv[1];
    char *target = argv[2];

    // initialize semaphore
    // MAX_THREADS threads can run concurrently
    sem_init(&thread_sem, 0, MAX_THREADS);

    //prepare root argument 
    thread_arg_t *root_arg = malloc(sizeof(thread_arg_t));
    if (!root_arg) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    root_arg->path = strdup(start);
    if (!root_arg->path) {
        fprintf(stderr, "Out of memory\n");
        free(root_arg);
        return 1;
    }
    root_arg->target = target;

    //start searching in the root directory
    search_dir(root_arg->path, root_arg->target);

    // cleanup
    free(root_arg->path);
    free(root_arg);

    // destroy semaphore
    sem_destroy(&thread_sem);

    return 0;
}



//THREAD ENTRY FUNCTION
void *thread_func(void *arg) {

    thread_arg_t *targ = (thread_arg_t*) arg;

    search_dir(targ->path, targ->target);

    // free memory for this thread's argument
    free(targ->path);
    free(targ);

    // IMPORTANT: signal semaphore → a thread slot is now free
    sem_post(&thread_sem);

    return NULL;
}




//DIRECTORY SEARCH
void search_dir(const char *path, const char *target) {

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Error: cannot open directory: %s\n", path);
        return;
    }

    struct dirent *entry;

    // dynamic array for child thread IDs
    pthread_t *child_threads = NULL;
    size_t child_count = 0;
    size_t child_cap = 0;

    // iterate through directory entries
    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char newpath[1024];
        snprintf(newpath, sizeof(newpath), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(newpath, &st) != 0)
            continue;

        //  FILE 
        if (S_ISREG(st.st_mode)) {
            if (strcmp(entry->d_name, target) == 0) {

                pthread_mutex_lock(&print_lock);
                printf("Found by thread %lu: %s\n",
                    (unsigned long)pthread_self(),
                    newpath);
                pthread_mutex_unlock(&print_lock);

            }
        }

        //  DIRECTORY 
        if (S_ISDIR(st.st_mode)) {

            // Wait for permission to create a new thread
            // If too many threads exist → waits here.
            sem_wait(&thread_sem);

            // prepare argument struct
            thread_arg_t *child_arg = malloc(sizeof(thread_arg_t));
            if (!child_arg) {
                sem_post(&thread_sem);
                continue;
            }

            child_arg->path = strdup(newpath);
            if (!child_arg->path) {
                free(child_arg);
                sem_post(&thread_sem);
                continue;
            }
            child_arg->target = (char *)target;

            pthread_t tid;
            int rc = pthread_create(&tid, NULL, thread_func, child_arg);

            if (rc != 0) {
                fprintf(stderr, "pthread_create failed for %s\n", newpath);
                free(child_arg->path);
                free(child_arg);
                sem_post(&thread_sem);
                continue;
            }

            // expand array if needed
            if (child_count + 1 > child_cap) {
                size_t new_cap = (child_cap == 0 ? 8 : child_cap * 2);
                pthread_t *tmp = realloc(child_threads, new_cap * sizeof(pthread_t));
                if (tmp) {
                    child_threads = tmp;
                    child_cap = new_cap;
                }
            }

            if (child_count < child_cap) {
                child_threads[child_count++] = tid;
            }
        }
    }

    //  JOIN ALL CHILD THREADS 
    for (size_t i = 0; i < child_count; i++) {
        pthread_join(child_threads[i], NULL);
    }

    free(child_threads);
    closedir(dir);
}
