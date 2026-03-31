#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "utils.h"

// ============================================================================
// Interpolation (Serial Code - Unchanged as per previous assignment)
// ============================================================================
void interpolation(double *mesh_value, Points *points) {
    memset(mesh_value, 0, GRID_X * GRID_Y * sizeof(double));

    for (int p = 0; p < NUM_Points; p++) {
        double xp = points[p].x / dx;
        double yp = points[p].y / dy;
        
        int i = (int)xp;
        int j = (int)yp;
        
        if (i >= NX) i = NX - 1;
        if (j >= NY) j = NY - 1;
        
        double wx = xp - i;
        double wy = yp - j;
        
        mesh_value[j * GRID_X + i]           += (1.0 - wx) * (1.0 - wy);
        mesh_value[j * GRID_X + i + 1]       += wx * (1.0 - wy);
        mesh_value[(j + 1) * GRID_X + i]     += (1.0 - wx) * wy;
        mesh_value[(j + 1) * GRID_X + i + 1] += wx * wy;
    }
}

// ============================================================================
// APPROACH 2: Immediate Replacement (Highly Optimized for Bonus Marks)
// ============================================================================

// Serial Immediate Replacement
void mover_immediate_serial(Points *points, double deltaX, double deltaY) {
    unsigned int seed = 9999; 
    
    for (int p = 0; p < NUM_Points; p++) {
        double rx = ((double)rand_r(&seed) / RAND_MAX) * 2.0 - 1.0;
        double ry = ((double)rand_r(&seed) / RAND_MAX) * 2.0 - 1.0;
        
        double new_x = points[p].x + rx * deltaX;
        double new_y = points[p].y + ry * deltaY;
        
        // If outside domain, replace immediately at the same memory location
        if (new_x < 0.0 || new_x > 1.0 || new_y < 0.0 || new_y > 1.0) {
            points[p].x = (double)rand_r(&seed) / RAND_MAX;
            points[p].y = (double)rand_r(&seed) / RAND_MAX;
        } else {
            points[p].x = new_x;
            points[p].y = new_y;
        }
    }
}

// Parallel Immediate Replacement (SCALES BEST)
void mover_immediate_parallel(Points *points, double deltaX, double deltaY) {
    #pragma omp parallel
    {
        // Thread-local seed to avoid massive RNG contention overhead
        unsigned int local_seed = 12345 + omp_get_thread_num();
        
        #pragma omp for schedule(static)
        for (int p = 0; p < NUM_Points; p++) {
            double rx = ((double)rand_r(&local_seed) / RAND_MAX) * 2.0 - 1.0;
            double ry = ((double)rand_r(&local_seed) / RAND_MAX) * 2.0 - 1.0;
            
            double new_x = points[p].x + rx * deltaX;
            double new_y = points[p].y + ry * deltaY;
            
            // Immediate replacement requires zero synchronization between threads!
            if (new_x < 0.0 || new_x > 1.0 || new_y < 0.0 || new_y > 1.0) {
                points[p].x = (double)rand_r(&local_seed) / RAND_MAX;
                points[p].y = (double)rand_r(&local_seed) / RAND_MAX;
            } else {
                points[p].x = new_x;
                points[p].y = new_y;
            }
        }
    }
}

// ============================================================================
// APPROACH 1: Deferred Insertion
// ============================================================================

// Parallel Deferred Insertion 
void mover_deferred_parallel(Points *points, double deltaX, double deltaY) {
    int deleted_count = 0;

    // Step 1: Move particles and mark deleted ones
    #pragma omp parallel
    {
        unsigned int local_seed = 12345 + omp_get_thread_num();
        
        // reduction clause safely tallies up the deleted count across threads
        #pragma omp for reduction(+:deleted_count) schedule(static)
        for (int p = 0; p < NUM_Points; p++) {
            double rx = ((double)rand_r(&local_seed) / RAND_MAX) * 2.0 - 1.0;
            double ry = ((double)rand_r(&local_seed) / RAND_MAX) * 2.0 - 1.0;
            
            double new_x = points[p].x + rx * deltaX;
            double new_y = points[p].y + ry * deltaY;
            
            if (new_x < 0.0 || new_x > 1.0 || new_y < 0.0 || new_y > 1.0) {
                points[p].x = -1.0; // Mark as a void
                deleted_count++;
            } else {
                points[p].x = new_x;
                points[p].y = new_y;
            }
        }
    }

    // Step 2: Compaction - Push voids to the end
    // (Done serially via a highly efficient O(N) two-pointer approach to avoid heavy atomic/prefix-sum overhead)
    int left = 0;
    int right = NUM_Points - 1;
    while (left <= right) {
        if (points[left].x != -1.0) {
            left++;
        } else if (points[right].x == -1.0) {
            right--;
        } else {
            // Swap valid right particle into empty left slot
            points[left] = points[right];
            points[right].x = -1.0;
            points[right].y = -1.0;
            left++;
            right--;
        }
    }

    // Step 3: Insert new particles at the end into the voids
    #pragma omp parallel
    {
        unsigned int local_seed = 9999 + omp_get_thread_num();
        
        #pragma omp for schedule(static)
        for (int p = NUM_Points - deleted_count; p < NUM_Points; p++) {
            points[p].x = (double)rand_r(&local_seed) / RAND_MAX;
            points[p].y = (double)rand_r(&local_seed) / RAND_MAX;
        }
    }
}
// Serial Deferred Insertion
void mover_deferred_serial(Points *points, double deltaX, double deltaY) {
    unsigned int seed = 9999;
    int deleted_count = 0;

    // Step 1: Move particles and mark deleted ones
    for (int p = 0; p < NUM_Points; p++) {
        double rx = ((double)rand_r(&seed) / RAND_MAX) * 2.0 - 1.0;
        double ry = ((double)rand_r(&seed) / RAND_MAX) * 2.0 - 1.0;
        
        double new_x = points[p].x + rx * deltaX;
        double new_y = points[p].y + ry * deltaY;
        
        // If outside domain, mark as a void
        if (new_x < 0.0 || new_x > 1.0 || new_y < 0.0 || new_y > 1.0) {
            points[p].x = -1.0; 
            deleted_count++;
        } else {
            points[p].x = new_x;
            points[p].y = new_y;
        }
    }

    // Step 2: Compaction - Push voids to the end using a two-pointer approach
    int left = 0;
    int right = NUM_Points - 1;
    while (left <= right) {
        if (points[left].x != -1.0) {
            left++;
        } else if (points[right].x == -1.0) {
            right--;
        } else {
            // Swap valid right particle into empty left slot
            points[left] = points[right];
            points[right].x = -1.0;
            points[right].y = -1.0;
            left++;
            right--;
        }
    }

    // Step 3: Insert new particles randomly at the end to replace the deleted ones
    for (int p = NUM_Points - deleted_count; p < NUM_Points; p++) {
        points[p].x = (double)rand_r(&seed) / RAND_MAX;
        points[p].y = (double)rand_r(&seed) / RAND_MAX;
    }
}
// ============================================================================
// Utility Methods (Provided)
// ============================================================================
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

// Assignment 04 Baseline: Parallel Mover (No deletion/insertion)
void mover_parallel(Points *points, double deltaX, double deltaY) {
    #pragma omp parallel
    {
        unsigned int local_seed = 12345 + omp_get_thread_num();
       
        #pragma omp for schedule(static)
        for (int p = 0; p < NUM_Points; p++) {
            double new_x, new_y;
            // Forces particles to stay inside by resampling
            do {
                double rx = ((double)rand_r(&local_seed) / RAND_MAX) * 2.0 - 1.0;
                double ry = ((double)rand_r(&local_seed) / RAND_MAX) * 2.0 - 1.0;
                new_x = points[p].x + rx * deltaX;
                new_y = points[p].y + ry * deltaY;
            } while (new_x < 0.0 || new_x > 1.0 || new_y < 0.0 || new_y > 1.0);
           
            points[p].x = new_x;
            points[p].y = new_y;
        }
    }
}

// Add to utils.cpp
void save_particles(Points *points, const char* filename) {
    FILE *fd = fopen(filename, "w");
    if (!fd) {
        printf("Error creating %s\n", filename);
        return;
    }
    
    fprintf(fd, "x,y\n"); // CSV Header
    for (int p = 0; p < NUM_Points; p++) {
        // Only save valid particles (avoids any -1.0 voids from the deferred approach)
        if (points[p].x >= 0.0 && points[p].x <= 1.0) {
            fprintf(fd, "%lf,%lf\n", points[p].x, points[p].y);
        }
    }
    fclose(fd);
}


