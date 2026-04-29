#ifndef INIT_H
#define INIT_H

#include <stdio.h>
#include <stdbool.h>

// Level 2 (SoA): Instead of an array of objects, we have an object of arrays
typedef struct {
    double *x;
    double *y;
    bool *is_void;
} Points;

// Global simulation parameters
extern int GRID_X, GRID_Y, NX, NY;
extern int NUM_Points, Maxiter;
extern double dx, dy;

// Initialization & I/O
void read_points(FILE *file, Points *points);
void initializepoints(Points *points);

#endif
