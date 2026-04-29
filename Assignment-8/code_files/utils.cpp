/*  =====================================================================
 *  utils.cpp  -  Hybrid MPI + OpenMP, Mesh Strip + Spatial Particle
 *                Decomposition (Assignment 8 — optimised for multi-node).
 *
 *  Strategy (replaces old full-mesh Allreduce approach)
 *  ----------------------------------------------------
 *   1. Mesh strip decomposition:
 *        Rank k owns CELL rows [local_row_start, local_row_end) and the
 *        corresponding NODE rows [local_row_start, local_row_end].
 *        mesh_value[] has (local_row_count + 1) node rows = local_strip_size.
 *        local_j = j - local_row_start  maps global cell row j to buffer row.
 *
 *   2. Spatial particle decomposition:
 *        read_local_points reads ALL particles but keeps only those whose
 *        y-coordinate falls in this rank's y-strip.  After each mover step,
 *        particles that drift into a neighbour's strip are migrated via
 *        point-to-point MPI sends.
 *
 *   3. Interpolation (particle → mesh):
 *        Per-thread private strip grids + manual chunked reduction (no atomics).
 *        Boundary node rows are shared with neighbours → add-exchange of
 *        GRID_X doubles per neighbour (vs. full-mesh Allreduce before).
 *
 *   4. Normalization:
 *        Local min/max (OpenMP reduction) + Allreduce of 2 doubles only.
 *
 *   5. Mover (mesh → particle):
 *        Purely local — each particle reads from its own rank's strip.
 *        Followed by particle migration to correct ranks.
 *
 *   6. Denormalization: local, no MPI.
 *
 *  Required changes in main.cpp
 *  ----------------------------
 *    • Declare and compute local_row_start / local_row_end / local_row_count.
 *    • Allocate mesh_value with GRID_X * (local_row_count + 1) doubles.
 *    • Allocate points arrays with NUM_Points slots (safe upper bound).
 *    • local_num_points is set by read_local_points; main must not set it.
 *    • utils.h must extern the three new row globals.
 *  ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#define OMPI_SKIP_MPICXX 1
#include <mpi.h>

#ifdef _OPENMP
    #include <omp.h>
#else
    static inline int omp_get_num_threads(void) { return 1; }
    static inline int omp_get_max_threads(void) { return 1; }
    static inline int omp_get_thread_num (void) { return 0; }
#endif

#include "init.h"
#include "utils.h"

/* =====================================================================
 *  Thread-private strip-grid buffer (cached across iterations).
 * ===================================================================== */
static double *tg_buffer   = NULL;
static int     tg_threads  = 0;
static int     tg_gridsize = 0;

static void ensure_tg_buffer(int num_threads, int grid_size)
{
    if (tg_buffer && tg_threads == num_threads && tg_gridsize == grid_size)
        return;
    free(tg_buffer);
    tg_buffer = NULL;
    size_t bytes = (size_t)num_threads * (size_t)grid_size * sizeof(double);
    tg_buffer = (double *)malloc(bytes);
    if (!tg_buffer) {
        fprintf(stderr,
                "Rank %d: failed to allocate %.2f MB thread-private grids\n",
                mpi_rank, bytes / (1024.0 * 1024.0));
        MPI_Abort(MPI_COMM_WORLD, 2);
    }
    tg_threads  = num_threads;
    tg_gridsize = grid_size;
}

void free_utils_buffers(void)
{
    free(tg_buffer);
    tg_buffer   = NULL;
    tg_threads  = 0;
    tg_gridsize = 0;
}

/* Saved for denormalization */
static double g_mesh_min = 0.0;
static double g_mesh_max = 0.0;

/* Max slots allocated for points arrays (set once in read_local_points) */
static int particle_capacity = 0;

/* =====================================================================
 *  read_local_points
 *  -----------------
 *  Every rank reads ALL NUM_Points particles from the binary file and
 *  keeps only those whose y-coordinate maps to its cell-row strip.
 *  sets global local_num_points on exit.
 *
 *  File layout:
 *      int NX, NY, NUM_Points, Maxiter   (16 bytes header)
 *      double x0, y0, x1, y1, ...        (NUM_Points pairs)
 * ===================================================================== */
void read_local_points(const char *filename, Points *points)
{
    /* y range that belongs to this rank's strip.
     * Cell j = floor(y / dy).  Rank owns cells [local_row_start, local_row_end).
     * Last rank extends to 1.0 (inclusive). */
    const double y_lo = local_row_start * dy;
    const double y_hi = (local_row_end >= NY) ? (1.0 + 1e-10) : (local_row_end * dy);

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Rank %d: cannot open '%s'\n", mpi_rank, filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Skip header — already parsed in main.cpp */
    if (fseek(fp, 4L * (long)sizeof(int), SEEK_SET) != 0) {
        fprintf(stderr, "Rank %d: fseek failed\n", mpi_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    particle_capacity = NUM_Points; /* main allocates NUM_Points slots */

    const int CHUNK = 65536;
    double *buf = (double *)malloc((size_t)CHUNK * 2 * sizeof(double));
    if (!buf) {
        fprintf(stderr, "Rank %d: I/O buffer alloc failed\n", mpi_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int remaining = NUM_Points;
    int idx = 0;

    while (remaining > 0) {
        int n = (remaining < CHUNK) ? remaining : CHUNK;
        size_t got = fread(buf, sizeof(double), (size_t)n * 2, fp);
        if ((int)got != n * 2) {
            fprintf(stderr, "Rank %d: short read (%zu of %d doubles)\n",
                    mpi_rank, got, n * 2);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        for (int k = 0; k < n; k++) {
            double x = buf[2 * k];
            double y = buf[2 * k + 1];
            if (y >= y_lo && y < y_hi) {
                if (idx >= particle_capacity) {
                    fprintf(stderr, "Rank %d: particle buffer overflow\n", mpi_rank);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                points->x[idx]       = x;
                points->y[idx]       = y;
                points->is_void[idx] = false;
                idx++;
            }
        }
        remaining -= n;
    }

    free(buf);
    fclose(fp);

    local_num_points = idx; /* set global so main / other functions see it */
}

/* =====================================================================
 *  interpolation  (particle -> mesh strip)
 *  ----------------------------------------
 *  Per-thread private strip grids + manual chunked reduction (no atomics).
 *  After reduction, boundary node rows are add-exchanged with neighbours:
 *    • Top boundary (local row 0 = global row local_row_start) shared with rank-1
 *    • Bottom boundary (local row local_row_count = global row local_row_end)
 *      shared with rank+1
 *  Cost: 2 * GRID_X doubles per neighbour  (vs. full GRID_X*GRID_Y Allreduce).
 * ===================================================================== */
void interpolation(double *mesh_value, Points *points)
{
    /* local_strip_size = number of DOUBLES in the local mesh buffer */
    const int local_strip_size = GRID_X * (local_row_count + 1);

    const double inv_dx = 1.0 / dx;
    const double inv_dy = 1.0 / dy;
    const double DX     = dx;
    const double DY     = dy;
    const int NX_loc    = NX;
    const int NY_loc    = NY;
    const int GX        = GRID_X;
    const int N_local   = local_num_points;
    const int RS        = local_row_start; /* offset for global->local row */

    double *px = points->x;
    double *py = points->y;
    bool   *pv = points->is_void;

    /* Zero the entire local strip */
    memset(mesh_value, 0, (size_t)local_strip_size * sizeof(double));

    const int num_threads = omp_get_max_threads();
    ensure_tg_buffer(num_threads, local_strip_size);

    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        const int nt  = omp_get_num_threads();
        double *my_grid = tg_buffer + (size_t)tid * (size_t)local_strip_size;

        memset(my_grid, 0, (size_t)local_strip_size * sizeof(double));

        /* -------- Particle scatter onto thread-private strip grid -------- */
        #pragma omp for schedule(static)
        for (int p = 0; p < N_local; p++) {
            if (pv[p]) continue;

            const double xp = px[p];
            const double yp = py[p];

            int i = (int)(xp * inv_dx);
            int j = (int)(yp * inv_dy);

            if      (i < 0)        i = 0;
            else if (i >= NX_loc)  i = NX_loc - 1;
            if      (j < 0)        j = 0;
            else if (j >= NY_loc)  j = NY_loc - 1;

            /* Map global cell row j to local buffer row */
            const int local_j = j - RS;
            /* Safety: skip if outside our strip (shouldn't happen after migration) */
            if (local_j < 0 || local_j >= local_row_count) continue;

            const double Xi   = (double)i * DX;
            const double Yj   = (double)j * DY;
            const double lx   = xp - Xi;
            const double ly   = yp - Yj;
            const double dxlx = DX - lx;
            const double dyly = DY - ly;

            const double w00 = dxlx * dyly;   /* (i  , j  ) */
            const double w10 = lx   * dyly;   /* (i+1, j  ) */
            const double w01 = dxlx * ly;     /* (i  , j+1) */
            const double w11 = lx   * ly;     /* (i+1, j+1) */

            const int idx = local_j * GX + i;
            my_grid[idx]          += w00;
            my_grid[idx + 1]      += w10;
            my_grid[idx + GX]     += w01;
            my_grid[idx + GX + 1] += w11;
        }
        /* Implicit barrier at end of 'omp for' */

        /* -------- Manual chunked reduction into mesh_value -------- */
        const int base  = local_strip_size / nt;
        const int rem   = local_strip_size % nt;
        const int first = tid * base + (tid < rem ? tid : rem);
        const int count = base + (tid < rem ? 1 : 0);
        const int last  = first + count;

        for (int k = first; k < last; k++)
            mesh_value[k] = tg_buffer[k];

        for (int t = 1; t < nt; t++) {
            const double *src = tg_buffer + (size_t)t * (size_t)local_strip_size;
            for (int k = first; k < last; k++)
                mesh_value[k] += src[k];
        }
    } /* implicit barrier */

    /* ================================================================
     *  Boundary node add-exchange with MPI neighbours
     *  -----------------------------------------------
     *  Node row 0              (global j_start) is shared with rank-1.
     *  Node row local_row_count (global j_end)  is shared with rank+1.
     *  We send our contribution and ADD what the neighbour sends back.
     *  Total cost: 2 * GRID_X doubles (per neighbour direction).
     * ================================================================ */
    if (mpi_size > 1) {
        int up   = (mpi_rank > 0)            ? mpi_rank - 1 : MPI_PROC_NULL;
        int down = (mpi_rank < mpi_size - 1) ? mpi_rank + 1 : MPI_PROC_NULL;

        /* Temporary receive buffer (reused for both directions) */
        double *recv_buf = (double *)calloc((size_t)GRID_X, sizeof(double));
        if (!recv_buf) { MPI_Abort(MPI_COMM_WORLD, 3); }

        /* --- Top boundary: exchange row 0 with rank above --- */
        /* We send our row 0 upward; rank-1 sends its bottom row (j_end) down */
        MPI_Sendrecv(
            mesh_value,        GRID_X, MPI_DOUBLE, up,   60,
            recv_buf,          GRID_X, MPI_DOUBLE, up,   61,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (up != MPI_PROC_NULL)
            for (int i = 0; i < GRID_X; i++)
                mesh_value[i] += recv_buf[i];

        memset(recv_buf, 0, (size_t)GRID_X * sizeof(double));

        /* --- Bottom boundary: exchange last row with rank below --- */
        MPI_Sendrecv(
            mesh_value + (size_t)local_row_count * GRID_X,
            GRID_X, MPI_DOUBLE, down, 61,
            recv_buf,
            GRID_X, MPI_DOUBLE, down, 60,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (down != MPI_PROC_NULL)
            for (int i = 0; i < GRID_X; i++)
                mesh_value[(size_t)local_row_count * GRID_X + i] += recv_buf[i];

        free(recv_buf);
    }
}

/* =====================================================================
 *  normalization : map strip values into [-1, 1]
 *  Local min/max found with OpenMP reduction, then Allreduce of just
 *  2 doubles gives the global range.  Far cheaper than before.
 * ===================================================================== */
void normalization(double *mesh_value)
{
    const int local_strip_size = GRID_X * (local_row_count + 1);

    double vmin =  DBL_MAX;
    double vmax = -DBL_MAX;

    #pragma omp parallel for reduction(min:vmin) reduction(max:vmax) schedule(static)
    for (int k = 0; k < local_strip_size; k++) {
        if (mesh_value[k] < vmin) vmin = mesh_value[k];
        if (mesh_value[k] > vmax) vmax = mesh_value[k];
    }

    /* Global min/max: only 2 doubles across the network */
    if (mpi_size > 1) {
        double gmin, gmax;
        MPI_Allreduce(&vmin, &gmin, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
        MPI_Allreduce(&vmax, &gmax, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        vmin = gmin;
        vmax = gmax;
    }

    g_mesh_min = vmin;
    g_mesh_max = vmax;

    const double range = vmax - vmin;
    if (range == 0.0) {
        #pragma omp parallel for schedule(static)
        for (int k = 0; k < local_strip_size; k++) mesh_value[k] = 0.0;
        return;
    }

    const double inv_range = 1.0 / range;
    #pragma omp parallel for schedule(static)
    for (int k = 0; k < local_strip_size; k++)
        mesh_value[k] = 2.0 * (mesh_value[k] - vmin) * inv_range - 1.0;
}

/* =====================================================================
 *  mover : reverse interpolation (mesh -> particle) + position update
 *  Purely local — each particle's cell lies inside this rank's strip,
 *  so mesh_value has all needed node rows.  No MPI in this phase.
 *  Followed by particle_migrate() to send drifted particles away.
 * ===================================================================== */

/* ---- Internal: migrate particles that drifted out of our y-strip ---- */
static void particle_migrate(Points *points)
{
    int local_need = 0;
    const double y_lo = local_row_start * dy;
    const double y_hi = (local_row_end >= NY) ? (1.0 + 1e-10) : (local_row_end * dy);
    for (int p = 0; p < local_num_points; p++) {
        if (!points->is_void[p] &&
            (points->y[p] < y_lo || points->y[p] >= y_hi)) {
            local_need = 1; break;
        }
    }
    int global_need = 0;
    MPI_Allreduce(&local_need, &global_need, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    if (global_need == 0) return;



    int up   = (mpi_rank > 0)            ? mpi_rank - 1 : MPI_PROC_NULL;
    int down = (mpi_rank < mpi_size - 1) ? mpi_rank + 1 : MPI_PROC_NULL;

    /* --- Count particles leaving via top / bottom --- */
    int send_up_cnt = 0, send_dn_cnt = 0;
    for (int p = 0; p < local_num_points; p++) {
        if (points->is_void[p]) continue;
        double y = points->y[p];
        if (y < y_lo  && up   != MPI_PROC_NULL) send_up_cnt++;
        if (y >= y_hi && down != MPI_PROC_NULL) send_dn_cnt++;
    }

    /* --- Build send buffers and mark those particles void locally --- */
    double *send_up = (send_up_cnt > 0)
                      ? (double *)malloc((size_t)send_up_cnt * 2 * sizeof(double)) : NULL;
    double *send_dn = (send_dn_cnt > 0)
                      ? (double *)malloc((size_t)send_dn_cnt * 2 * sizeof(double)) : NULL;
    int ui = 0, di = 0;

    for (int p = 0; p < local_num_points; p++) {
        if (points->is_void[p]) continue;
        double y = points->y[p];
        if (y < y_lo && up != MPI_PROC_NULL) {
            send_up[ui * 2]     = points->x[p];
            send_up[ui * 2 + 1] = points->y[p];
            points->is_void[p] = true;   /* free this slot */
            ui++;
        } else if (y >= y_hi && down != MPI_PROC_NULL) {
            send_dn[di * 2]     = points->x[p];
            send_dn[di * 2 + 1] = points->y[p];
            points->is_void[p] = true;
            di++;
        }
    }

    /* --- Exchange particle counts with neighbours --- */
    int recv_from_up = 0, recv_from_dn = 0;
    {
        MPI_Request reqs[4];
        int nreqs = 0;
        if (up   != MPI_PROC_NULL) {
            MPI_Isend(&send_up_cnt, 1, MPI_INT, up,   70, MPI_COMM_WORLD, &reqs[nreqs++]);
            MPI_Irecv(&recv_from_up, 1, MPI_INT, up,  71, MPI_COMM_WORLD, &reqs[nreqs++]);
        }
        if (down != MPI_PROC_NULL) {
            MPI_Isend(&send_dn_cnt, 1, MPI_INT, down, 71, MPI_COMM_WORLD, &reqs[nreqs++]);
            MPI_Irecv(&recv_from_dn, 1, MPI_INT, down,70, MPI_COMM_WORLD, &reqs[nreqs++]);
        }
        MPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
    }

    /* --- Exchange particle data with neighbours --- */
    double *recv_up = (recv_from_up > 0)
                      ? (double *)malloc((size_t)recv_from_up * 2 * sizeof(double)) : NULL;
    double *recv_dn = (recv_from_dn > 0)
                      ? (double *)malloc((size_t)recv_from_dn * 2 * sizeof(double)) : NULL;
    {
        MPI_Request reqs[4];
        int nreqs = 0;
        if (up   != MPI_PROC_NULL) {
            MPI_Isend(send_up, send_up_cnt * 2, MPI_DOUBLE, up,   80, MPI_COMM_WORLD, &reqs[nreqs++]);
            MPI_Irecv(recv_up, recv_from_up * 2, MPI_DOUBLE, up,  81, MPI_COMM_WORLD, &reqs[nreqs++]);
        }
        if (down != MPI_PROC_NULL) {
            MPI_Isend(send_dn, send_dn_cnt * 2, MPI_DOUBLE, down, 81, MPI_COMM_WORLD, &reqs[nreqs++]);
            MPI_Irecv(recv_dn, recv_from_dn * 2, MPI_DOUBLE, down,80, MPI_COMM_WORLD, &reqs[nreqs++]);
        }
        MPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
    }

    /* --- Insert received particles into void slots, then extend array --- */
    int recv_total = recv_from_up + recv_from_dn;
    if (recv_total > 0) {
        /* Flatten received pairs into temporary arrays */
        double *rx = (double *)malloc((size_t)recv_total * sizeof(double));
        double *ry = (double *)malloc((size_t)recv_total * sizeof(double));
        for (int k = 0; k < recv_from_up; k++) {
            rx[k] = recv_up[k * 2];
            ry[k] = recv_up[k * 2 + 1];
        }
        for (int k = 0; k < recv_from_dn; k++) {
            rx[recv_from_up + k] = recv_dn[k * 2];
            ry[recv_from_up + k] = recv_dn[k * 2 + 1];
        }

        /* Fill existing void slots first */
        int fill_idx = 0;
        for (int p = 0; p < local_num_points && fill_idx < recv_total; p++) {
            if (points->is_void[p]) {
                points->x[p]       = rx[fill_idx];
                points->y[p]       = ry[fill_idx];
                points->is_void[p] = false;
                fill_idx++;
            }
        }
        /* Append any remaining beyond current local_num_points */
        int new_count = local_num_points;
        for (int k = fill_idx; k < recv_total; k++) {
            if (new_count >= particle_capacity) {
                fprintf(stderr, "Rank %d: particle buffer overflow in migration\n", mpi_rank);
                MPI_Abort(MPI_COMM_WORLD, 4);
            }
            points->x[new_count]       = rx[k];
            points->y[new_count]       = ry[k];
            points->is_void[new_count] = false;
            new_count++;
        }
        local_num_points = new_count;

        free(rx); free(ry);
    }

    free(send_up); free(send_dn);
    free(recv_up); free(recv_dn);
}

void mover(double *mesh_value, Points *points)
{
    const double inv_dx = 1.0 / dx;
    const double inv_dy = 1.0 / dy;
    const double DX     = dx;
    const double DY     = dy;
    const int NX_loc    = NX;
    const int NY_loc    = NY;
    const int GX        = GRID_X;
    const int N_local   = local_num_points;
    const int RS        = local_row_start;

    double *px = points->x;
    double *py = points->y;
    bool   *pv = points->is_void;

    #pragma omp parallel for schedule(static)
    for (int p = 0; p < N_local; p++) {
        if (pv[p]) continue;

        const double xp = px[p];
        const double yp = py[p];

        int i = (int)(xp * inv_dx);
        int j = (int)(yp * inv_dy);

        if      (i < 0)        i = 0;
        else if (i >= NX_loc)  i = NX_loc - 1;
        if      (j < 0)        j = 0;
        else if (j >= NY_loc)  j = NY_loc - 1;

        const int local_j = j - RS;
        if (local_j < 0 || local_j >= local_row_count) continue;

        const double Xi   = (double)i * DX;
        const double Yj   = (double)j * DY;
        const double lx   = xp - Xi;
        const double ly   = yp - Yj;
        const double dxlx = DX - lx;
        const double dyly = DY - ly;

        const double w00 = dxlx * dyly;
        const double w10 = lx   * dyly;
        const double w01 = dxlx * ly;
        const double w11 = lx   * ly;

        const int idx = local_j * GX + i;
        const double F = w00 * mesh_value[idx]
                       + w10 * mesh_value[idx + 1]
                       + w01 * mesh_value[idx + GX]
                       + w11 * mesh_value[idx + GX + 1];

        const double xn = xp + F * DX;
        const double yn = yp + F * DY;

        if (xn < 0.0 || xn > 1.0 || yn < 0.0 || yn > 1.0) {
            pv[p] = true;
        } else {
            px[p] = xn;
            py[p] = yn;
        }
    }

    /* Send particles that drifted into a neighbour's y-strip */
    if (mpi_size > 1)
        particle_migrate(points);
}

/* =====================================================================
 *  denormalization : inverse of normalization.
 *  g_mesh_min / g_mesh_max are globally consistent (set from global
 *  min/max in normalization), so no extra MPI needed here.
 * ===================================================================== */
void denormalization(double *mesh_value)
{
    const int    local_strip_size = GRID_X * (local_row_count + 1);
    const double range            = g_mesh_max - g_mesh_min;
    const double vmin             = g_mesh_min;

    if (range == 0.0) {
        #pragma omp parallel for schedule(static)
        for (int k = 0; k < local_strip_size; k++) mesh_value[k] = vmin;
        return;
    }

    const double half_range = 0.5 * range;
    #pragma omp parallel for schedule(static)
    for (int k = 0; k < local_strip_size; k++)
        mesh_value[k] = (mesh_value[k] + 1.0) * half_range + vmin;
}

/* =====================================================================
 *  save_mesh : gather all strips at rank 0 and write output.txt.
 *  Each rank sends its cell rows (local_row_count node rows from
 *  local_row_start to local_row_end - 1 exclusive).
 *  The last rank also sends its final node row (= global GRID_Y - 1).
 * ===================================================================== */
void save_mesh(double *mesh_value)
{
    /* Each rank sends local_row_count rows, except the last rank which
     * sends local_row_count + 1 rows (including the shared top node row). */
    int my_rows     = (mpi_rank < mpi_size - 1) ? local_row_count : (local_row_count + 1);
    int my_send_sz  = my_rows * GRID_X;

    int *recv_sizes = NULL;
    int *displs     = NULL;
    double *full    = NULL;

    if (mpi_rank == 0) {
        recv_sizes = (int *)malloc((size_t)mpi_size * sizeof(int));
        displs     = (int *)malloc((size_t)mpi_size * sizeof(int));
    }

    MPI_Gather(&my_send_sz, 1, MPI_INT,
               recv_sizes,  1, MPI_INT, 0, MPI_COMM_WORLD);

    if (mpi_rank == 0) {
        int total = 0;
        for (int r = 0; r < mpi_size; r++) {
            displs[r] = total;
            total    += recv_sizes[r];
        }
        full = (double *)malloc((size_t)total * sizeof(double));
        if (!full) {
            fprintf(stderr, "Rank 0: full mesh alloc failed in save_mesh\n");
            MPI_Abort(MPI_COMM_WORLD, 5);
        }
    }

    /* mesh_value[0] = node row local_row_start, send from start of buffer */
    MPI_Gatherv(mesh_value, my_send_sz, MPI_DOUBLE,
                full, recv_sizes, displs, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    if (mpi_rank == 0) {
        FILE *fp = fopen("output.txt", "w");
        if (!fp) {
            fprintf(stderr, "Rank 0: cannot create output.txt\n");
        } else {
            for (int j = 0; j < GRID_Y; j++) {
                for (int i = 0; i < GRID_X; i++) {
                    fprintf(fp, "%.6f", full[j * GRID_X + i]);
                    if (i < GRID_X - 1) fputc(' ', fp);
                }
                fputc('\n', fp);
            }
            fclose(fp);
        }
        free(full);
        free(recv_sizes);
        free(displs);
    }
}

/* =====================================================================
 *  void_count : local inactive particle count.
 *  Summed globally via MPI_Reduce in main.cpp.
 * ===================================================================== */
long long void_count(Points *points)
{
    long long cnt     = 0;
    const int N_local = local_num_points;
    bool *pv          = points->is_void;

    #pragma omp parallel for reduction(+:cnt) schedule(static)
    for (int p = 0; p < N_local; p++)
        if (pv[p]) cnt++;

    return cnt;
}