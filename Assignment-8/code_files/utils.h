#ifndef UTILS_H
#define UTILS_H

#include "init.h"

/* ====================================================================
 *  MPI + decomposition globals (defined in main.cpp)
 * ==================================================================== */
extern int mpi_rank;
extern int mpi_size;
extern int local_num_points; // number of particles owned by this rank
extern int local_start;      // index into the full particle array
extern int local_row_start;
extern int local_row_end;
extern int local_row_count;
/* ====================================================================
 *  Core pipeline (hybrid MPI + OpenMP)
 * ==================================================================== */
void interpolation(double *mesh_value, Points *points);
void normalization(double *mesh_value);
void mover(double *mesh_value, Points *points);
void denormalization(double *mesh_value);

/* ====================================================================
 *  I/O helpers
 * ==================================================================== */
void read_local_points(const char *filename, Points *points);
void save_mesh(double *mesh_value);

/* ====================================================================
 *  Diagnostics / cleanup
 * ==================================================================== */
long long void_count(Points *points);
void free_utils_buffers(void);

#endif /* UTILS_H */
