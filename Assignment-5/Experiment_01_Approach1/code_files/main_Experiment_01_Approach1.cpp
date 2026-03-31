#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "init.h"
#include "utils.h"

int GRID_X, GRID_Y, NX, NY;
int NUM_Points, Maxiter;
double dx, dy;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <NX> <NY> <NUM_Points>\n", argv[0]);
        return -1;
    }

    NX = atoi(argv[1]);
    NY = atoi(argv[2]);
    NUM_Points = atoi(argv[3]);
   
    Maxiter = 10;
    GRID_X = NX + 1;
    GRID_Y = NY + 1;
    dx = 1.0 / NX;
    dy = 1.0 / NY;

    Points *points = (Points *)malloc(NUM_Points * sizeof(Points));
    double *mesh_value = (double *)malloc(GRID_X * GRID_Y * sizeof(double));

    if (!points || !mesh_value) {
        printf("Memory allocation failed!\n");
        return -1;
    }

    srand(42);
    initializepoints(points);

    // ============================================================================
    // Serial Deferred Insertion Approach
    // ============================================================================
    double total_interp_time_def = 0.0;
    double total_mover_time_def = 0.0;

    for (int iter = 0; iter < Maxiter; iter++) {
        double t_start = omp_get_wtime();
        interpolation(mesh_value, points);
        total_interp_time_def += (omp_get_wtime() - t_start);

        t_start = omp_get_wtime();
        mover_deferred_serial(points, dx, dy);
        total_mover_time_def += (omp_get_wtime() - t_start);
    }
    double total_time_def = total_interp_time_def + total_mover_time_def;

    printf("Deferred|%f|%f|%f\n", total_interp_time_def, total_mover_time_def, total_time_def);

    free(points);
    free(mesh_value);

    return 0;
}