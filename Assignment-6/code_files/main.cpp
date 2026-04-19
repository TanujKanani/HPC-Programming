#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
    #include <omp.h>
#else
    #include <time.h>
    static double omp_get_wtime() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ts.tv_nsec * 1e-9;
    }
#endif

#include "init.h"
#include "utils.h"

// Global variables
int GRID_X, GRID_Y, NX, NY;
int NUM_Points, Maxiter;
double dx, dy;

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    // Open binary file for reading
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        printf("Error opening input file\n");
        exit(1);
    }

    // Read problem parameters from the binary file header
    fread(&NX, sizeof(int), 1, file);
    fread(&NY, sizeof(int), 1, file);
    fread(&NUM_Points, sizeof(int), 1, file);
    fread(&Maxiter, sizeof(int), 1, file);

    GRID_X = NX + 1;
    GRID_Y = NY + 1;
    dx = 1.0 / NX;
    dy = 1.0 / NY;

    // Allocate memory for grid and Points
    double *mesh_value = (double *)calloc(GRID_X * GRID_Y, sizeof(double));
    Points *points = (Points *)calloc(NUM_Points, sizeof(Points));

    double total_time = 0.0;

    for (int iter = 0; iter < Maxiter; iter++) {
        // Read scattered points from file for this iteration
        read_points(file, points);

        // TRUE WALL-CLOCK TIMING
        double start = omp_get_wtime();
        
        // Perform highly optimized parallel interpolation (Sorting + Privatization)
        interpolation(mesh_value, points);
        
        double end = omp_get_wtime();

        total_time += (end - start);
    }

    // Save the final iteration's mesh to verify accuracy
    save_mesh(mesh_value);
    
    // Calculate and print the timings for your report graphs
    printf("Total interpolation time (parallel) = %lf seconds\n", total_time);
    printf("Average time per iteration = %lf seconds\n", total_time / Maxiter);

    // Free memory
    free(mesh_value);
    free(points);
    fclose(file);

    return 0;
}