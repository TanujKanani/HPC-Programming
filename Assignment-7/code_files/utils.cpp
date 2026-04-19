#include "utils.h"
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double min_val, max_val;

void interpolation(double *mesh_value, Points *points) {
  int grid_size = GRID_X * GRID_Y;
  memset(mesh_value, 0, grid_size * sizeof(double));

  double reciprocal_x = (double)NX;
  double reciprocal_y = (double)NY;
  double area = dx * dy;

  int nthreads = omp_get_max_threads();

  double *priv_meshes = (double *)calloc(nthreads * grid_size, sizeof(double));

#pragma omp parallel
  {
    int thr_id = omp_get_thread_num();
    double *thr_grid = &priv_meshes[thr_id * grid_size];

#pragma omp for schedule(static)
    for (int n = 0; n < NUM_Points; n++) {
      if (points->is_void[n])
        continue;

      double px = points->x[n];
      double py = points->y[n];

      int ci = (int)(px * reciprocal_x);
      int cj = (int)(py * reciprocal_y);

      ci = (ci >= NX) ? NX - 1 : ((ci < 0) ? 0 : ci);
      cj = (cj >= NY) ? NY - 1 : ((cj < 0) ? 0 : cj);

      double fx = (px * reciprocal_x) - ci;
      double fy = (py * reciprocal_y) - cj;

      double cx = 1.0 - fx;
      double cy = 1.0 - fy;

      double wt00 = cx * cy * area;
      double wt10 = fx * cy * area;
      double wt01 = cx * fy * area;
      double wt11 = fx * fy * area;

      int idx = cj * GRID_X + ci;

      thr_grid[idx] += wt00;
      thr_grid[idx + 1] += wt10;
      thr_grid[idx + GRID_X] += wt01;
      thr_grid[idx + GRID_X + 1] += wt11;
    }
  }

#pragma omp parallel for schedule(static)
  for (int k = 0; k < grid_size; k++) {
    double total = 0.0;
    for (int th = 0; th < nthreads; th++) {
      total += priv_meshes[th * grid_size + k];
    }
    mesh_value[k] += total;
  }

  free(priv_meshes);
}

void normalization(double *mesh_value) {
  int grid_size = GRID_X * GRID_Y;
  min_val = mesh_value[0];
  max_val = mesh_value[0];

#pragma omp parallel
  {
    double thr_min = mesh_value[0];
    double thr_max = mesh_value[0];

#pragma omp for schedule(static)
    for (int k = 0; k < grid_size; k++) {
      if (mesh_value[k] < thr_min)
        thr_min = mesh_value[k];
      if (mesh_value[k] > thr_max)
        thr_max = mesh_value[k];
    }

#pragma omp critical
    {
      if (thr_min < min_val)
        min_val = thr_min;
      if (thr_max > max_val)
        max_val = thr_max;
    }
  }

  double span = max_val - min_val;
  if (span == 0.0)
    span = 1.0;

#pragma omp parallel for schedule(static)
  for (int k = 0; k < grid_size; k++) {
    mesh_value[k] = 2.0 * (mesh_value[k] - min_val) / span - 1.0;
  }
}

void mover(double *mesh_value, Points *points) {
  double reciprocal_x = (double)NX;
  double reciprocal_y = (double)NY;
  double area = dx * dy;

#pragma omp parallel for schedule(static)
  for (int n = 0; n < NUM_Points; n++) {
    if (points->is_void[n])
      continue;

    double px = points->x[n];
    double py = points->y[n];

    int ci = (int)(px * reciprocal_x);
    int cj = (int)(py * reciprocal_y);

    ci = (ci >= NX) ? NX - 1 : ((ci < 0) ? 0 : ci);
    cj = (cj >= NY) ? NY - 1 : ((cj < 0) ? 0 : cj);

    double fx = (px * reciprocal_x) - ci;
    double fy = (py * reciprocal_y) - cj;

    double cx = 1.0 - fx;
    double cy = 1.0 - fy;

    double wt00 = cx * cy * area;
    double wt10 = fx * cy * area;
    double wt01 = cx * fy * area;
    double wt11 = fx * fy * area;

    int idx = cj * GRID_X + ci;

    double field = wt00 * mesh_value[idx] + wt10 * mesh_value[idx + 1] +
                   wt01 * mesh_value[idx + GRID_X] +
                   wt11 * mesh_value[idx + GRID_X + 1];

    points->x[n] += field * dx;
    points->y[n] += field * dy;

    if (points->x[n] < 0.0 || points->x[n] > 1.0 || points->y[n] < 0.0 ||
        points->y[n] > 1.0) {
      points->is_void[n] = true;
    }
  }
}

void denormalization(double *mesh_value) {
  int grid_size = GRID_X * GRID_Y;
  double span = max_val - min_val;
  if (span == 0.0)
    span = 1.0;

#pragma omp parallel for schedule(static)
  for (int k = 0; k < grid_size; k++) {
    mesh_value[k] = (mesh_value[k] + 1.0) * span / 2.0 + min_val;
  }
}

long long int void_count(Points *points) {
  long long int count = 0;
#pragma omp parallel for reduction(+ : count)
  for (int k = 0; k < NUM_Points; k++) {
    count += (int)points->is_void[k];
  }
  return count;
}

void save_mesh(double *mesh_value) {
  FILE *fp = fopen("Mesh.out", "w");
  if (!fp) {
    printf("Error creating Mesh.out\n");
    exit(1);
  }
  for (int r = 0; r < GRID_Y; r++) {
    for (int c = 0; c < GRID_X; c++) {
      fprintf(fp, "%lf ", mesh_value[r * GRID_X + c]);
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
}
