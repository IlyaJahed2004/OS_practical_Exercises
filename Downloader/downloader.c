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



// Function to get the size of a file (in bytes) using a HEAD request
long get_file_size(const char *url) {
    CURL *curl = curl_easy_init();    // Create a CURL handle
    double filesize = 0.0;            // libcurl stores size as a double
    CURLcode res;

    if (curl) {
        // Set the target URL
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Make a HEAD request (no body)
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

        // Include headers in the response (optional)
        curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

        // Perform the request
        res = curl_easy_perform(curl);

        // If successful, retrieve the Content-Length
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
        } else {
            fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
            filesize = -1;  // indicate failure
        }

        // Clean up and free resources
        curl_easy_cleanup(curl);
    }

    return (long)filesize; // Return file size (or -1 if failed)
}



void *download_part(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;
    CURL *curl = curl_easy_init();
    FILE *fp;
    char range[64], outname[64];

    sprintf(outname, "part_%d.bin", data->part_no);
    fp = fopen(outname, "wb");

    sprintf(range, "%ld-%ld", data->start, data->end - 1);

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, data->url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_RANGE, range);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    fclose(fp);
    printf("Thread %d downloaded bytes %ld-%ld\n", data->part_no, data->start, data->end - 1);
    return NULL;
}



// Function to download a specific byte range of a file (used by each thread)
void *download_part(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;  // Cast argument to ThreadData
    CURL *curl = curl_easy_init();                       // Initialize CURL handle
    FILE *fp;
    char range[64], outname[64];

    // Create output filename like "part_0.bin"
    sprintf(outname, "part_%d.bin", data->part_no);
    fp = fopen(outname, "wb");

    // Define the byte range to download (e.g., "0-999")
    sprintf(range, "%ld-%ld", data->start, data->end - 1);

    if (curl) {
        // Set file URL
        curl_easy_setopt(curl, CURLOPT_URL, data->url);

        // Set output file pointer
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // Use default write callback
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);

        // Specify byte range for partial download
        curl_easy_setopt(curl, CURLOPT_RANGE, range);

        // Perform the request
        curl_easy_perform(curl);

        // Clean up CURL resources
        curl_easy_cleanup(curl);
    }

    fclose(fp); // Close output file
    printf("Thread %d downloaded bytes %ld-%ld\n", data->part_no, data->start, data->end - 1);

    return NULL; // Thread returns nothing
}




// Function to merge all downloaded parts into one final file
void merge_files(const char *filename, int num_parts) {
    FILE *final = fopen(filename, "wb");   // Open final output file
    char part_name[64];
    FILE *part;
    char buffer[8192];                     // Temporary buffer for data transfer
    size_t n;

    // Loop through all parts in correct order
    for (int i = 0; i < num_parts; i++) {
        sprintf(part_name, "part_%d.bin", i);   // Build part filename (e.g., part_0.bin)
        part = fopen(part_name, "rb");          // Open part file for reading

        // Read from part and write into final file
        while ((n = fread(buffer, 1, sizeof(buffer), part)) > 0)
            fwrite(buffer, 1, n, final);

        fclose(part);       // Close part file
        remove(part_name);  // Delete part file after merging
    }

    fclose(final); // Close final merged file
}


