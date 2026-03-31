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
        printf("Usage: %s <NX> <NY> <NUM_THREADS>\n", argv[0]);
        return -1;
    }

    NX = atoi(argv[1]);
    NY = atoi(argv[2]);
    int num_threads = atoi(argv[3]);

    NUM_Points = 14000000; 
    Maxiter = 10;
    
    GRID_X = NX + 1;
    GRID_Y = NY + 1;
    dx = 1.0 / NX;
    dy = 1.0 / NY;

    omp_set_num_threads(num_threads);

    Points *points = (Points *)malloc(NUM_Points * sizeof(Points));
    double *mesh_value = (double *)malloc(GRID_X * GRID_Y * sizeof(double));
    Points *points_backup = (Points *)malloc(NUM_Points * sizeof(Points));

    if (!points || !mesh_value || !points_backup) {
        printf("Memory allocation failed!\n");
        return -1;
    }

    srand(42); 
    initializepoints(points);
    memcpy(points_backup, points, NUM_Points * sizeof(Points));

    // ============================================================================
    // Baseline Parallel Mover (Assignment 04)
    // ============================================================================
    double base_interp_time = 0.0, base_mover_time = 0.0;
    for (int iter = 0; iter < Maxiter; iter++) {
        double t_start = omp_get_wtime();
        interpolation(mesh_value, points);
        base_interp_time += (omp_get_wtime() - t_start);

        t_start = omp_get_wtime();
        mover_parallel(points, dx, dy);
        base_mover_time += (omp_get_wtime() - t_start);
    }
    double base_total_time = base_interp_time + base_mover_time;

    // ============================================================================
    // Deferred Insertion Parallel Mover
    // ============================================================================
    memcpy(points, points_backup, NUM_Points * sizeof(Points));
    double def_interp_time = 0.0, def_mover_time = 0.0;
    for (int iter = 0; iter < Maxiter; iter++) {
        double t_start = omp_get_wtime();
        interpolation(mesh_value, points);
        def_interp_time += (omp_get_wtime() - t_start);

        t_start = omp_get_wtime();
        mover_deferred_parallel(points, dx, dy);
        def_mover_time += (omp_get_wtime() - t_start);
    }
    double def_total_time = def_interp_time + def_mover_time;

    printf("Baseline|%f|%f|%f\n", base_interp_time, base_mover_time, base_total_time);
    printf("Deferred|%f|%f|%f\n", def_interp_time, def_mover_time, def_total_time);

    free(points);
    free(points_backup);
    free(mesh_value);

    return 0;
}