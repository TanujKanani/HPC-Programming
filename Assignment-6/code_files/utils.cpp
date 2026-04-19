#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<omp.h>
#include "utils.h"

void interpolation(double *mesh_value, Points *points) {
    int grid_size = GRID_X * GRID_Y;

    #pragma omp parallel
    {
        // Each thread gets a private mesh — eliminates all race conditions without atomics
        double *local_mesh = (double *)calloc(grid_size, sizeof(double));

        #pragma omp for schedule(static)
        for (int p = 0; p < NUM_Points; p++) {
            double px = points[p].x;
            double py = points[p].y;

            // Grid cell indices
            int i = (int)(px / dx);
            int j = (int)(py / dy);

            // Clamp to valid range so (i+1, j+1) stays in bounds
            if (i >= GRID_X - 1) i = GRID_X - 2;
            if (j >= GRID_Y - 1) j = GRID_Y - 2;
            if (i < 0) i = 0;
            if (j < 0) j = 0;

            double lx = px - i * dx;
            double ly = py - j * dy;

            double w00 = (dx - lx) * (dy - ly);
            double w10 = lx * (dy - ly);
            double w01 = (dx - lx) * ly;
            double w11 = lx * ly;

            // f_i = 1, so just accumulate weights
            local_mesh[j * GRID_X + i]         += w00;
            local_mesh[j * GRID_X + (i + 1)]   += w10;
            local_mesh[(j + 1) * GRID_X + i]   += w01;
            local_mesh[(j + 1) * GRID_X + (i + 1)] += w11;
        }

        // Reduce all private meshes into the shared mesh
        #pragma omp critical
        {
            for (int k = 0; k < grid_size; k++) {
                mesh_value[k] += local_mesh[k];
            }
        }

        free(local_mesh);
    }
}

void save_mesh(double *mesh_value) {
    FILE *fd = fopen("Mesh.out", "w");
    if (!fd) {
        printf("Error creating Mesh.out\n");
        exit(1);
    }
    for (int i = 0; i < GRID_Y; i++) {
        for (int j = 0; j < GRID_X; j++) {
            fprintf(fd, "%lf ", mesh_value[i * GRID_X + j]);
        }
        fprintf(fd, "\n");
    }
    fclose(fd);
}