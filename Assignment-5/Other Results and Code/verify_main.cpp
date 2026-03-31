#include <stdio.h>
#include <stdlib.h>
#include "init.h"
#include "utils.h"

// Define the global variables
int GRID_X, GRID_Y, NX, NY;
int NUM_Points, Maxiter;
double dx, dy;

int main() {
    // Hardcode safe parameters for the verification plot
    NX = 250;
    NY = 100;
    NUM_Points = 100000; // 10^5 is perfect for plotting
    Maxiter = 10;
    
    GRID_X = NX + 1;
    GRID_Y = NY + 1;
    dx = 1.0 / NX;
    dy = 1.0 / NY;

    // Allocate memory
    Points *points = (Points *)malloc(NUM_Points * sizeof(Points));
    double *mesh_value = (double *)malloc(GRID_X * GRID_Y * sizeof(double));

    if (!points || !mesh_value) {
        printf("Memory allocation failed!\n");
        return -1;
    }

    // Initialize
    srand(42); 
    initializepoints(points);

    // Run the mover for 10 iterations
    for (int iter = 0; iter < Maxiter; iter++) {
        // We can just use the serial immediate mover for this quick test
        mover_immediate_serial(points, dx, dy);
    }

    // Save the particles using the function we added to utils.cpp
    save_particles(points, "final_particles.csv");
    printf("Successfully saved %d particles to final_particles.csv\n", NUM_Points);

    // Clean up
    free(points);
    free(mesh_value);

    return 0;
}