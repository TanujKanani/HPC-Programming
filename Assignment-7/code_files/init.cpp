#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "init.h"

void read_points(FILE *file, Points *points) {
    for (int i = 0; i < NUM_Points; i++) {
        fread(&(points->x[i]), sizeof(double), 1, file);
        fread(&(points->y[i]), sizeof(double), 1, file);
        points->is_void[i] = false;   
    }
}
