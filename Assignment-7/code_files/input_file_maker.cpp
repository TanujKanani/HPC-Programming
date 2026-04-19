#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INPUT_FILENAME "input.bin"

void generate_input_file(int NX, int NY, int NUM_Points, int Maxiter) {

    FILE *fp = fopen(INPUT_FILENAME, "wb");
    if (fp == NULL) {
        perror("Error: Unable to create file input.bin\n");
        exit(EXIT_FAILURE);
    }

    fwrite(&NX, sizeof(int), 1, fp);
    fwrite(&NY, sizeof(int), 1, fp);
    fwrite(&NUM_Points, sizeof(int), 1, fp);
    fwrite(&Maxiter, sizeof(int), 1, fp);

    srand((unsigned int)time(NULL));

    for (int iter = 0; iter < Maxiter; iter++) {
        for (int i = 0; i < NUM_Points; i++) {

            double x = (double)rand() / RAND_MAX;
            double y = (double)rand() / RAND_MAX;

            fwrite(&x, sizeof(double), 1, fp);
            fwrite(&y, sizeof(double), 1, fp);
        }
    }   

    fclose(fp);

    printf("Input file '%s' generated successfully.\n", INPUT_FILENAME);
}

int main() {

    int NX, NY;
    int NUM_Points;
    int Maxiter;

    printf("Enter grid dimensions (NX NY): ");
    scanf("%d %d", &NX, &NY);

    printf("Enter number of particles: ");
    scanf("%d", &NUM_Points);

    printf("Enter number of iterations: ");
    scanf("%d", &Maxiter);

    generate_input_file(NX, NY, NUM_Points, Maxiter);

    return 0;
}