#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <pthread.h>    
#include <curl/curl.h> 

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
    // Check for correct number of arguments
    if (argc < 3) {
        printf("Usage: %s <url> <num_threads>\n", argv[0]);
        return 1;
    }

    const char *url = argv[1];         // File URL
    int num_threads = atoi(argv[2]);   // Number of threads to use

    // Limit threads to MAX_THREADS
    if (num_threads > MAX_THREADS)
        num_threads = MAX_THREADS;

    // Initialize CURL globally
    curl_global_init(CURL_GLOBAL_ALL);

    // Get total file size from server
    long filesize = get_file_size(url);
    printf("File size: %ld bytes\n", filesize);

    // Calculate the size of each part
    long part_size = filesize / num_threads;

    pthread_t threads[MAX_THREADS];
    struct ThreadData tdata[MAX_THREADS];

    // Create threads for each part
    for (int i = 0; i < num_threads; i++) {
        tdata[i].start = i * part_size;
        tdata[i].end = (i == num_threads - 1) ? filesize : (i + 1) * part_size;
        strcpy(tdata[i].url, url);
        tdata[i].part_no = i;

        pthread_create(&threads[i], NULL, download_part, &tdata[i]);
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    // Merge all downloaded parts into one final file
    merge_files("output_file", num_threads);

    printf("Download complete!\n");

    // Clean up CURL resources
    curl_global_cleanup();

    return 0;
}


long get_file_size(const char *url) {
    CURL *curl;
    CURLcode res;
    double filesize = -1.0;

    curl = curl_easy_init();
    if (curl) {
        // Set the target URL
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Use HEAD request (only get headers, not file content)
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

        // Follow redirects if the URL points somewhere else
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Perform the request
        res = curl_easy_perform(curl);

        // If successful, extract file size info
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &filesize);
        } else {
            // Print error message if something goes wrong
            printf("Error: %s\n", curl_easy_strerror(res));
        }

        // Clean up CURL resources
        curl_easy_cleanup(curl);
    }

    // Check if file size was obtained successfully
    if (filesize < 0)
        printf("Error: Could not get file size\n");

    return (long)filesize;
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





