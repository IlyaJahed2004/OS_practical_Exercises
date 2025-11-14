// Enable POSIX extensions (for functions like strdup, realpath, pthreads)
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>   // Needed for opendir(), readdir(), closedir()
#include <sys/stat.h>   // for stat()


// Structure that will later be passed to each thread
typedef struct {
    char *path;    // Starting directory path
    char *target;  // Target filename to search for
} thread_arg_t;


// Function prototype for directory listing
void search_dir(const char *path, const char *target);


int main(int argc, char *argv[]) {

    // Expect exactly 2 arguments: starting directory + target filename  
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <start_directory> <target_filename>\n", argv[0]);
        return 1;
    }

    char *start = argv[1];   // Starting directory
    char *target = argv[2];  // Filename to search for

    // Allocate memory for the thread argument structure
    thread_arg_t *root_arg = malloc(sizeof(thread_arg_t));
    if (!root_arg) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    // Duplicate the starting directory path so we can safely free it later
    root_arg->path = strdup(start);
    if (!root_arg->path) {
        fprintf(stderr, "Out of memory\n");
        free(root_arg);
        return 1;
    }

    // The target name does not need duplication (constant during program)
    root_arg->target = target;

    // Debug output for this step (to verify everything works)
    printf("Root path prepared: %s\n", root_arg->path);
    printf("Target filename: %s\n", root_arg->target);

    //call directory listing function ---
    search_dir(root_arg->path,root_arg->target);

    // Free allocated memory (later this will be handled differently)
    free(root_arg->path);
    free(root_arg);

    return 0;
}

void *thread_func(void *arg) {

    thread_arg_t *targ = (thread_arg_t*) arg;

    search_dir(targ->path, targ->target);

    // free memory after done
    free(targ->path);
    free(targ);

    return NULL;
}



void search_dir(const char *path, const char *target) {

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Error: cannot open directory: %s\n", path);
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Build full path
        char newpath[1024];
        snprintf(newpath, sizeof(newpath), "%s/%s", path, entry->d_name);

        // Get info about the entry
        struct stat st;
        if (stat(newpath, &st) != 0)
            continue;

        //If it's a file → check match
        if (S_ISREG(st.st_mode)) {           // Is normal file?
            if (strcmp(entry->d_name, target) == 0) {
                printf("FOUND: %s\n", newpath);
            }
        }

        if (S_ISDIR(st.st_mode)) {

            // 1) allocate memory for new thread argument
            thread_arg_t *child_arg = malloc(sizeof(thread_arg_t));

            // 2) fill it
            child_arg->path = strdup(newpath);
            child_arg->target = target;

            // 3) create a new thread
            pthread_t tid;
            pthread_create(&tid, NULL, thread_func, child_arg);

        }

    }

    closedir(dir);
}



