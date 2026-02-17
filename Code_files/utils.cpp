#include <stdlib.h>
#include "utils.h"


void matrix_multiplication_ijk(double** m1, double** m2, double** result, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double sum = 0.0;
            for (int k = 0; k < N; k++) {
                sum += m1[i][k] * m2[k][j];
            }
            result[i][j] = sum;
        }
    }
}

void matrix_multiplication_ikj(double** m1, double** m2, double** result, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) result[i][j] = 0.0;
        for (int k = 0; k < N; k++) {
            double r = m1[i][k];
            for (int j = 0; j < N; j++) {
                result[i][j] += r * m2[k][j];
            }
        }
    }
}


void matrix_multiplication_jik(double** m1, double** m2, double** result, int N) {
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            double sum = 0.0;
            for (int k = 0; k < N; k++) {
                sum += m1[i][k] * m2[k][j];
            }
            result[i][j] = sum;
        }
    }
}

void matrix_multiplication_jki(double** m1, double** m2, double** result, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) result[i][j] = 0.0;
        
    for (int j = 0; j < N; j++) {
        for (int k = 0; k < N; k++) {
            double r = m2[k][j];
            for (int i = 0; i < N; i++) {
                result[i][j] += m1[i][k] * r;
            }
        }
    }
}

void matrix_multiplication_kij(double** m1, double** m2, double** result, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) result[i][j] = 0.0;

    for (int k = 0; k < N; k++) {
        for (int i = 0; i < N; i++) {
            double r = m1[i][k];
            for (int j = 0; j < N; j++) {
                result[i][j] += r * m2[k][j];
            }
        }
    }
}

void matrix_multiplication_kji(double** m1, double** m2, double** result, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) result[i][j] = 0.0;

    for (int k = 0; k < N; k++) {
        for (int j = 0; j < N; j++) {
            double r = m2[k][j];
            for (int i = 0; i < N; i++) {
                result[i][j] += m1[i][k] * r;
            }
        }
    }
}


void transpose(double** m, double** mt, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            mt[i][j] = m[j][i];
        }
    }
}

void transposed_matrix_multiplication(double** m1, double** mt, double** result, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double sum = 0.0;
            for (int k = 0; k < N; k++) {
                sum += m1[i][k] * mt[j][k]; 
            }
            result[i][j] = sum;
        }
    }
}

void block_matrix_multiplication(double** m1, double** m2, double** result, int B, int N) {
    for(int i=0; i<N; i++) 
        for(int j=0; j<N; j++) 
            result[i][j] = 0.0;

    for (int i = 0; i < N; i += B) {
        for (int k = 0; k < N; k += B) { 
            for (int j = 0; j < N; j += B) {
                // Inner block multiplication with IKJ order for speed
                for (int ii = i; ii < i + B && ii < N; ii++) {
                    for (int kk = k; kk < k + B && kk < N; kk++) {
                        double r = m1[ii][kk];
                        for (int jj = j; jj < j + B && jj < N; jj++) {
                            result[ii][jj] += r * m2[kk][jj];
                        }
                    }
                }
            }
        }
    }
}
