#ifndef INIT_H
#define INIT_H

#include <stdio.h>
#include <stdbool.h>

typedef struct {
    double *x;
    double *y;
    bool *is_void;
} Points;

extern int GRID_X, GRID_Y, NX, NY;
extern int NUM_Points, Maxiter;
extern double dx, dy;

void read_points(FILE *file, Points *points);
void initializepoints(Points *points);

#endif
