#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fcntl.h>      
#include <sys/mman.h>   
#include <sys/stat.h>   
#include <unistd.h>


#define N 6
#define M 6

#define K 2
#define L 2


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

            // For each column in this row
            for (int col = 0; col < M; col++) {

                int w_max = -999999;
                int w_min = 999999;

                // Loop over KxL window
                for (int a = 0; a < K; a++) {
                    for (int b = 0; b < L; b++) {

                        int rr = row + a;
                        int cc = col + b;

                        if (rr < 0 || rr >= N || cc < 0 || cc >= M)
                            continue;

                        int val = M1[rr][cc];

                        if (val > w_max) w_max = val;
                        if (val < w_min) w_min = val;
                    }
                }

                //FINAL STORE after full K×L scan
                M2[row * M + col] = w_max - w_min;
            }

            printf("[Child PID %d] Finished computing row %d\n", getpid(), row);
            exit(0);
        }

    }  

    // Parent waits for all children
    for (int i = 0; i < N; i++) {
        wait(NULL);
    }

    printf("All child processes finished.\n\n");

    return 0;
}
