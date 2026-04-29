/*  =====================================================================
 *  main.cpp  -  Hybrid MPI + OpenMP driver for Assignment 8
 *               (Updated for mesh strip + spatial particle decomposition)
 * ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OMPI_SKIP_MPICXX 1
#include <mpi.h>

#ifdef _OPENMP
    #include <omp.h>
#endif

#include "init.h"
#include "utils.h"

/* ------------ Simulation globals (declared extern in init.h) ---------- */
int    GRID_X, GRID_Y, NX, NY;
int    NUM_Points, Maxiter;
double dx, dy;

/* ------------ MPI globals (declared extern in utils.h) ---------------- */
int mpi_rank         = 0;
int mpi_size         = 1;
int local_num_points = 0;   /* set by read_local_points, updated by migration */
int local_start      = 0;   /* kept for compatibility, not used in strip mode */

/* ------------ NEW: mesh strip globals (declared extern in utils.h) ---- */
int local_row_start  = 0;   /* first cell row owned by this rank            */
int local_row_end    = 0;   /* one-past-last cell row                        */
int local_row_count  = 0;   /* = local_row_end - local_row_start            */

int main(int argc, char **argv)
{
    /* ---- MPI init ---- */
    int provided = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    if (argc != 2) {
        if (mpi_rank == 0)
            fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    /* ---- Header broadcast ---- */
    int header[4] = {0, 0, 0, 0};
    if (mpi_rank == 0) {
        FILE *file = fopen(argv[1], "rb");
        if (!file) { fprintf(stderr, "Error opening '%s'\n", argv[1]); MPI_Abort(MPI_COMM_WORLD,1); }
        fread(&header[0], sizeof(int), 1, file);
        fread(&header[1], sizeof(int), 1, file);
        fread(&header[2], sizeof(int), 1, file);
        fread(&header[3], sizeof(int), 1, file);
        fclose(file);
    }
    MPI_Bcast(header, 4, MPI_INT, 0, MPI_COMM_WORLD);

    NX         = header[0];
    NY         = header[1];
    NUM_Points = header[2];
    Maxiter    = header[3];

    GRID_X = NX + 1;
    GRID_Y = NY + 1;
    dx     = 1.0 / NX;
    dy     = 1.0 / NY;

    /* ================================================================
     *  Mesh strip decomposition  (NEW — replaces particle decomposition)
     *  Decompose CELL rows [0, NY) across ranks.
     * ================================================================ */
    {
        int base = NY / mpi_size;
        int rem  = NY % mpi_size;
        local_row_count = base + (mpi_rank < rem ? 1 : 0);
        local_row_start = mpi_rank * base + (mpi_rank < rem ? mpi_rank : rem);
        local_row_end   = local_row_start + local_row_count;
    }

    /* ---- Allocate local mesh strip: (local_row_count+1) node rows ---- */
    /* +1 because cells [j_start, j_end) span nodes [j_start, j_end]     */
    int local_strip_nodes = local_row_count + 1;
    double *mesh_value = (double *)calloc(
        (size_t)GRID_X * local_strip_nodes, sizeof(double));
    if (!mesh_value) {
        fprintf(stderr, "Rank %d: mesh allocation failed\n", mpi_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* ---- Allocate particle SoA: worst case = all particles on one rank ---- */
    Points *points  = (Points *)malloc(sizeof(Points));
    points->x       = (double *)calloc((size_t)NUM_Points, sizeof(double));
    points->y       = (double *)calloc((size_t)NUM_Points, sizeof(double));
    points->is_void = (bool   *)calloc((size_t)NUM_Points, sizeof(bool));
    if (!points->x || !points->y || !points->is_void) {
        fprintf(stderr, "Rank %d: particle allocation failed\n", mpi_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* ---- Read particles (each rank reads all, keeps its y-strip) ---- */
    read_local_points(argv[1], points);
    /* local_num_points is now set by read_local_points */

    /* ---- Timers ---- */
    double t_int = 0.0, t_norm = 0.0, t_move = 0.0, t_denorm = 0.0;

    MPI_Barrier(MPI_COMM_WORLD);
    const double t_pipeline_start = MPI_Wtime();

    /* ---- Main loop ---- */
    for (int iter = 0; iter < Maxiter; iter++) {
        double t0 = MPI_Wtime();
        interpolation(mesh_value, points);
        double t1 = MPI_Wtime();
        normalization(mesh_value);
        double t2 = MPI_Wtime();
        mover(mesh_value, points);
        double t3 = MPI_Wtime();
        denormalization(mesh_value);
        double t4 = MPI_Wtime();

        t_int    += (t1 - t0);
        t_norm   += (t2 - t1);
        t_move   += (t3 - t2);
        t_denorm += (t4 - t3);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    const double wall_time = MPI_Wtime() - t_pipeline_start;

    /* ---- Output ---- */
    save_mesh(mesh_value);

    /* ---- Aggregate per-phase max times ---- */
    double max_int = 0.0, max_norm = 0.0, max_move = 0.0, max_denorm = 0.0;
    MPI_Reduce(&t_int,    &max_int,    1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_norm,   &max_norm,   1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_move,   &max_move,   1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_denorm, &max_denorm, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    long long local_voids = void_count(points);
    long long total_voids = 0;
    MPI_Reduce(&local_voids, &total_voids, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (mpi_rank == 0) {
        printf("MPI Ranks                  = %d\n", mpi_size);
#ifdef _OPENMP
        printf("OpenMP Threads / Rank      = %d\n", omp_get_max_threads());
#endif
        printf("Grid (cells)               = %d x %d\n", NX, NY);
        printf("Grid (nodes)               = %d x %d\n", GRID_X, GRID_Y);
        printf("Total Particles            = %d\n", NUM_Points);
        printf("Iterations                 = %d\n", Maxiter);
        printf("Total Interpolation Time   = %lf seconds\n", max_int);
        printf("Total Normalization Time   = %lf seconds\n", max_norm);
        printf("Total Mover Time           = %lf seconds\n", max_move);
        printf("Total Denormalization Time = %lf seconds\n", max_denorm);
        printf("Total Algorithm Time       = %lf seconds\n", wall_time);
        printf("Total Number of Voids      = %lld\n", total_voids);
        printf("CSV,%lf,%lf,%lf\n", wall_time, max_int, max_move);
    }

    /* ---- Cleanup ---- */
    free_utils_buffers();
    free(mesh_value);
    free(points->x);
    free(points->y);
    free(points->is_void);
    free(points);

    MPI_Finalize();
    return 0;
}