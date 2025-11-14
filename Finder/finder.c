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
void search_dir(const char *path);



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
    search_dir(root_arg->path);

    // Free allocated memory (later this will be handled differently)
    free(root_arg->path);
    free(root_arg);

    return 0;
}

void search_dir(const char *path) {

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Error: cannot open directory: %s\n", path);
        return;
    }

    printf("Entering directory: %s\n", path);

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        printf(" - %s\n", entry->d_name);

        // Build full path
        char newpath[1024];
        snprintf(newpath, sizeof(newpath), "%s/%s", path, entry->d_name);

        // Use stat() to check if entry is a directory
        struct stat st;
        if (stat(newpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            // This is a directory → recursive call
            search_dir(newpath);
        }
    }

    closedir(dir);
}


