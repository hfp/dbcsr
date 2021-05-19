/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#ifndef DBCSR_ACC_BENCH_H
#define DBCSR_ACC_BENCH_H

#include <stdlib.h>
#include <assert.h>

#if !defined(MIN)
# define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif
#if !defined(MAX)
# define MAX(A, B) ((B) < (A) ? (A) : (B))
#endif


#define INIT_MAT(ELEM_TYPE, SEED, MAT, M, N, SCALE) do { \
  const double seed1 = (SCALE) * (SEED) + (SCALE); \
  int i, j; \
  for (i = 0; i < (N); ++i) { \
    for (j = 0; j < (M); ++j) { \
      const int k = i * (M) + j; \
      ((ELEM_TYPE*)(MAT))[k] = (ELEM_TYPE)(seed1 * (k + 1)); \
    } \
  } \
} while(0)


/* artificial stack-setup for DBCSR/ACC benchmarks */
static void init_stack(int* stack, int stack_size,
  int mn, int mk, int kn, int nc, int na, int nb)
{
  /* navg matrix products are accumulated into a C-matrix */
  int navg = stack_size / nc;
  int nimb = MAX(1, navg - 4); /* imbalance */
  int i = 0, c = 0, ntop = 0;
  assert(0 < nc && nc <= stack_size);
  while (i < stack_size) {
    const int next = c + 1;
    ntop += navg + (rand() % (2 * nimb) - nimb);
    if (stack_size < ntop) ntop = stack_size;
    for (;i < ntop; ++i) { /* setup one-based indexes */
      const int a = rand() % na, b = rand() % nb;
      *stack++ = a * mk + 1; /* A-index */
      *stack++ = b * kn + 1; /* B-index */
      *stack++ = c * mn + 1; /* C-index */
    }
    if (next < nc) c = next;
  }
}

#endif /*DBCSR_ACC_BENCH_H*/
