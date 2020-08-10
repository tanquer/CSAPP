/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void trans_32(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, k;
    int val0, val1, val2, val3, val4, val5, val6, val7;

    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (k = i; k < i + 8; k++) {
                val0 = A[k][j];
                val1 = A[k][j+1];
                val2 = A[k][j+2];
                val3 = A[k][j+3];
                val4 = A[k][j+4];
                val5 = A[k][j+5];
                val6 = A[k][j+6];
                val7 = A[k][j+7];

                B[j][k] = val0;
                B[j+1][k] = val1;
                B[j+2][k] = val2;
                B[j+3][k] = val3;
                B[j+4][k] = val4;
                B[j+5][k] = val5;
                B[j+6][k] = val6;
                B[j+7][k] = val7;
            }
        }
    }
}

void trans_64(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, k, l;
    int val0, val1, val2, val3, val4, val5, val6, val7;

    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (k = i; k < i + 4; k++) {
                val0 = A[k][j];
                val1 = A[k][j+1];
                val2 = A[k][j+2];
                val3 = A[k][j+3];
                val4 = A[k][j+4];
                val5 = A[k][j+5];
                val6 = A[k][j+6];
                val7 = A[k][j+7];

                B[j][k] = val0;
                B[j+1][k] = val1;
                B[j+2][k] = val2;
                B[j+3][k] = val3;
                B[j][k+4] = val4;
                B[j+1][k+4] = val5;
                B[j+2][k+4] = val6;
                B[j+3][k+4] = val7;
            }
            
            for (l = j; l < j + 4; l++) {
                val0 = A[k][l];
                val1 = A[k+1][l];
                val2 = A[k+2][l];
                val3 = A[k+3][l];

                val4 = B[l][k];
                val5 = B[l][k+1];
                val6 = B[l][k+2];
                val7 = B[l][k+3];

                B[l][k] = val0;
                B[l][k+1] = val1;
                B[l][k+2] = val2;
                B[l][k+3] = val3;

                B[l+4][k-4] = val4;
                B[l+4][k-3] = val5;
                B[l+4][k-2] = val6;
                B[l+4][k-1] = val7;
            }
            
            for (; l < j + 8; l++) {
                val0 = A[k][l];
                val1 = A[k+1][l];
                val2 = A[k+2][l];
                val3 = A[k+3][l];

                B[l][k] = val0;
                B[l][k+1] = val1;
                B[l][k+2] = val2;
                B[l][k+3] = val3;
            }
        }
    }
}

void trans_61(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, k, l;

    for (i = 0; i < N; i += 16) {
        for (j = 0; j < M; j += 16) {
            for (k = i; k < i + 16 && k < N; k++) {
                for (l = j; l < j + 16 && l < M; l++) {
                    B[l][k] = A[k][l];
                }
            }
        }
    }
}

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (N == 32) {
        trans_32(M, N, A, B);
    } else if (N == 64) {
        trans_64(M, N, A, B);
    } else {
        trans_61(M, N, A, B);
    }  
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

