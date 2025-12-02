#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define N 6
#define M 6

#define K 2   // Window height
#define L 4   // Window width

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
    printf("\n");

    //  Determine size of output matrix M2 
    int out_rows = (N + K - 1) / K;   // ceil(N/K)
    int out_cols = (M + L - 1) / L;   // ceil(M/L)

    printf("M2 size will be %d x %d\n\n", out_rows, out_cols);

    //  Shared Memory for M2 
    int fd = shm_open("/lgp_shm", O_CREAT | O_RDWR, 0666);
    if (fd == -1) { perror("shm_open failed"); return 1; }

    size_t size = sizeof(int) * out_rows * out_cols;
    if (ftruncate(fd, size) == -1) { perror("ftruncate failed"); return 1; }

    int *M2 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (M2 == MAP_FAILED) { perror("mmap failed"); return 1; }

    for (int i = 0; i < out_rows * out_cols; i++)
        M2[i] = 0;

    //  Create CHILD PROCESSES (one per row of M2) 

    printf("Creating %d child processes (one per output row)...\n\n", out_rows);

    for (int out_r = 0; out_r < out_rows; out_r++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            return 1;
        }

        if (pid == 0) {
            //  CHILD CODE 
            printf("[PID %d] Processing output row %d...\n", getpid(), out_r);

            int start_r = out_r * K;

            for (int out_c = 0; out_c < out_cols; out_c++) {

                int start_c = out_c * L;

                int w_max = -999999;
                int w_min =  999999;

                // Scan K×L window of M1
                for (int a = 0; a < K; a++) {
                    for (int b = 0; b < L; b++) {

                        int rr = start_r + a;
                        int cc = start_c + b;

                        if (rr >= N || cc >= M)
                            continue;

                        int val = M1[rr][cc];
                        if (val > w_max) w_max = val;
                        if (val < w_min) w_min = val;
                    }
                }

                // Store into M2
                M2[out_r * out_cols + out_c] = w_max - w_min;
            }

            printf("[PID %d] Finished output row %d\n", getpid(), out_r);
            exit(0);
        }
    }

    //  Parent waits 
    for (int i = 0; i < out_rows; i++)
        wait(NULL);

    printf("\nAll child processes finished.\n\n");

    //  PRINT M2 (Final Output)
    printf("Final M2 (Local Gradient Pooling):\n");
    for (int i = 0; i < out_rows; i++) {
        for (int j = 0; j < out_cols; j++) {
            printf("%d ", M2[i * out_cols + j]);
        }
        printf("\n");
    }

    return 0;
}