#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fcntl.h>      
#include <sys/mman.h>   
#include <sys/stat.h>   
#include <unistd.h>


#define N 6
#define M 6

int main() {

    int M1[N][M];   

    srand(time(NULL)); 

    printf("Matrix M1 (random values 1..9):\n");

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {

            M1[i][j] = 1 + rand() % 9; 

            printf("%d ", M1[i][j]);
        }
        printf("\n");
    }

    //Step 2: Create POSIX shared memory for M2

    // Create shared memory object
    int fd = shm_open("/lgp_shm", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        return 1;
    }

    // Set size of shared memory (for N*M integers)
    size_t size = sizeof(int) * N * M;
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate failed");
        return 1;
    }

    //  Map shared memory into process address space
    int *M2 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (M2 == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    // Initialize M2 to all zeros
    for (int i = 0; i < N * M; i++) {
        M2[i] = 0;
    }

    // Print M2 to verify
    printf("Shared memory M2 initialized to zeros:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            printf("%d ", M2[i * M + j]);
        }
        printf("\n");
    }
    printf("\n");


    // Step 3: Create child processes, one per row of the matrix

    printf("Creating %d child processes...\n", N);

    for (int row = 0; row < N; row++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            return 1;
        }

        if (pid == 0) {
            // Child process code
            printf("[Child PID %d] I am computing row %d...\n", getpid(), row);

            // Later in Step 4:
            // We will compute M2[row * M + col] here.

            exit(0);  // Child must exit so it doesn't continue loop
        }
    }

    // Parent waits for all children
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }
    
    printf("All child processes finished.\n\n");


    return 0;
}
