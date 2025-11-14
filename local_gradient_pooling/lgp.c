#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

    return 0;
}
