#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
    #include <omp.h>
#else
    #include <time.h>
    double omp_get_wtime() {
        return (double)clock() / CLOCKS_PER_SEC;
    }
#endif

#include "init.h"
#include "utils.h"

int GRID_X, GRID_Y, NX, NY;
int NUM_Points, Maxiter;
double dx, dy;

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("Error opening input file\n");
        exit(1);
    }

    fread(&NX, sizeof(int), 1, fp);
    fread(&NY, sizeof(int), 1, fp);

    fread(&NUM_Points, sizeof(int), 1, fp);
    fread(&Maxiter, sizeof(int), 1, fp);

    GRID_X = NX + 1;
    GRID_Y = NY + 1;
    dx = 1.0 / NX;
    dy = 1.0 / NY;

    double *mesh_value = (double *) calloc(GRID_X * GRID_Y, sizeof(double));
    
    Points *points = (Points *) malloc(sizeof(Points));
    points->x = (double *) calloc(NUM_Points, sizeof(double));
    points->y = (double *) calloc(NUM_Points, sizeof(double));
    points->is_void = (bool *) calloc(NUM_Points, sizeof(bool));

    double time_interp = 0.0;
    double time_norm = 0.0;
    double time_move = 0.0;
    double time_denorm = 0.0;

    read_points(fp, points);

    for (int iter = 0; iter < Maxiter; iter++) {

        double t0 = omp_get_wtime();
        interpolation(mesh_value, points);
        double t1 = omp_get_wtime();
        
        normalization(mesh_value);
        double t2 = omp_get_wtime();

        mover(mesh_value, points);
        double t3 = omp_get_wtime();

        denormalization(mesh_value);
        double t4 = omp_get_wtime();

        time_interp  += (t1 - t0);
        time_norm    += (t2 - t1);
        time_move    += (t3 - t2);
        time_denorm  += (t4 - t3);
    }

    save_mesh(mesh_value);
    printf("Total Interpolation Time = %lf seconds\n", time_interp);
    printf("Total Normalization Time = %lf seconds\n", time_norm);
    printf("Total Mover Time = %lf seconds\n", time_move);
    printf("Total Denormalization Time = %lf seconds\n", time_denorm);
    printf("Total Algorithm Time = %lf seconds\n", time_interp + time_norm + time_move + time_denorm);
    printf("Total Number of Voids = %lld\n", void_count(points));
    
    printf("CSV,%lf\n", time_interp + time_norm + time_move + time_denorm);
    
    free(mesh_value);
    
    free(points->x);
    free(points->y);
    free(points->is_void);
    free(points);
    
    fclose(fp);

    return 0;
}
