#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "init.h"

// Random particle initialization (optional)
void initializepoints(Points *points) {
    for (int i = 0; i < NUM_Points; i++) {
        points[i].x = (double)rand() / RAND_MAX;
        points[i].y = (double)rand() / RAND_MAX;
    }
}

// Read particle positions from binary file (OPTIMIZED)
void read_points(FILE *file, Points *points) {
    // Instead of looping millions of times, read the entire array in one massive chunk.
    // This removes the massive I/O bottleneck from the simulation.
    size_t elements_read = fread(points, sizeof(Points), NUM_Points, file);
    if (elements_read != NUM_Points) {
        printf("Warning: Expected to read %d points, but got %zu\n", NUM_Points, elements_read);
    }
}