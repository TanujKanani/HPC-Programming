#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "init.h"
#include "utils.h"

int GRID_X, GRID_Y, NX, NY;
int NUM_Points, Maxiter;
double dx, dy;

int main() {
    NX = 1000;
    NY = 400;
    Maxiter = 10;
    NUM_Points = 14000000; // 14 million
    GRID_X = NX + 1;
    GRID_Y = NY + 1;
    dx = 1.0 / NX;
    dy = 1.0 / NY;

    omp_set_num_threads(4); // Fixed to 4 threads

    double *mesh_value = (double *) calloc(GRID_X * GRID_Y, sizeof(double));
    Points *points = (Points *) calloc(NUM_Points, sizeof(Points));

    // CSV Header
    printf("Mode,Iteration,InterpTime,MoverTime,TotalTime\n");

    // --- SERIAL TEST ---
    initializepoints(points);
    for (int iter = 0; iter < Maxiter; iter++) {
        double start_int = omp_get_wtime();
        interpolation(mesh_value, points);
        double end_int = omp_get_wtime();
        
        double start_mov = omp_get_wtime();
        mover_serial(points, dx, dy);
        double end_mov = omp_get_wtime();
        
        double interp_time = end_int - start_int;
        double move_time = end_mov - start_mov;
        
        printf("Serial,%d,%lf,%lf,%lf\n", iter+1, interp_time, move_time, interp_time + move_time);
    }

    // --- PARALLEL TEST ---
    initializepoints(points); // Reset points
    for (int iter = 0; iter < Maxiter; iter++) {
        double start_int = omp_get_wtime();
        interpolation(mesh_value, points);
        double end_int = omp_get_wtime();
        
        double start_mov = omp_get_wtime();
        mover_parallel(points, dx, dy);
        double end_mov = omp_get_wtime();
        
        double interp_time = end_int - start_int;
        double move_time = end_mov - start_mov;
        
        printf("Parallel,%d,%lf,%lf,%lf\n", iter+1, interp_time, move_time, interp_time + move_time);
    }

    free(mesh_value);
    free(points);
    return 0;
}
