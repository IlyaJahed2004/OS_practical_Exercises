#include <stdio.h>      // Standard input/output functions (printf, fopen, etc.)
#include <stdlib.h>     // General-purpose utilities (malloc, atoi, etc.)
#include <string.h>     // String handling functions (strcpy, strlen, etc.)
#include <pthread.h>    // POSIX threads for concurrent downloads
#include <curl/curl.h>  // libcurl for HTTP/HTTPS requests

#define MAX_THREADS 8    // Maximum number of threads allowed

// Struct to store data for each thread (URL, byte range, and part number)
struct ThreadData {
    char url[512];      // URL of the file to download
    long start;         // Starting byte position for this thread
    long end;           // Ending byte position for this thread
    int part_no;        // Thread number (part index)
};

// Function prototypes (declarations)
// Each function will be implemented in later stages
void *download_part(void *arg);                  // Download a specific part of the file
long get_file_size(const char *url);             // Get total file size via HTTP HEAD
void merge_files(const char *filename, int num_parts);  // Combine downloaded parts

int main(int argc, char *argv[]) {
    // Entry point of the program
    // Will handle argument parsing, thread creation, and file merging
}
