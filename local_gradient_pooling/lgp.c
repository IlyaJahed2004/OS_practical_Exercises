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

    // 1) Create shared memory object
    int fd = shm_open("/lgp_shm", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        return 1;
    }

    // 2) Set size of shared memory (for N*M integers)
    size_t size = sizeof(int) * N * M;
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate failed");
        return 1;
    }

    // 3) Map shared memory into process address space
    int *M2 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (M2 == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    // 4) Initialize M2 to all zeros
    for (int i = 0; i < N * M; i++) {
        M2[i] = 0;
    }

    // 5) Print M2 to verify
    printf("Shared memory M2 initialized to zeros:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            printf("%d ", M2[i * M + j]);
        }
        printf("\n");
    }
    printf("\n");

    return 0;
}
