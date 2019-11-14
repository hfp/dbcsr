/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "libsmm.h"
#include <stdlib.h>
#include <assert.h>

#if !defined(LIBSMM_TRANSPOSE_BLOCKDIM_MAX)
# define LIBSMM_TRANSPOSE_BLOCKDIM_MAX 80
#endif

/** Naive implementation */
#define LIBSMM_TRANSPOSE(TYPE, STACK, STACKSIZE, M, N, MATRIX) { \
  int s, i, j; \
  for (s = 0; s < (STACKSIZE); ++s) { \
    TYPE tmp[LIBSMM_TRANSPOSE_BLOCKDIM_MAX*LIBSMM_TRANSPOSE_BLOCKDIM_MAX]; \
    TYPE *const mat = &((MATRIX)[(STACK)[s]]); \
    for (i = 0; i < (M); ++i) { \
      for (j = 0; j < (N); ++j) { \
        tmp[i*(N)+j] = mat[j*(M)+i]; \
      } \
    } \
    for (i = 0; i < (M); ++i) { \
      for (j = 0; j < (N); ++j) { \
        mat[i*(N)+j] = tmp[i*(N)+j]; \
      } \
    } \
  } \
}


#if defined(__cplusplus)
extern "C" {
#endif

#if defined(ACC_OPENMP_OFFLOAD)
# pragma omp declare target
#endif
int libsmm_acc_transpose_d(const int* dev_trs_stack, int offset, int nblks, double* dev_data, int m, int n)
{
  const int *const stack = dev_trs_stack + offset;
  int result;
#if defined(LIBSMM_TRANSPOSE_BLOCKDIM_MAX)
  if (LIBSMM_TRANSPOSE_BLOCKDIM_MAX >= m && LIBSMM_TRANSPOSE_BLOCKDIM_MAX >= n) {
    LIBSMM_TRANSPOSE(double, stack, nblks, m, n, dev_data);
    result = EXIT_SUCCESS;
  }
  else
#endif
  { /* TODO: well-performing library based implementation */
    result = EXIT_FAILURE;
  }
  return result;
}
#if defined(ACC_OPENMP_OFFLOAD)
# pragma omp end declare target
#endif


#if defined(ACC_OPENMP_OFFLOAD)
# pragma omp declare target
#endif
int libsmm_acc_transpose_s(const int* dev_trs_stack, int offset, int nblks, float* dev_data, int m, int n)
{
  const int *const stack = dev_trs_stack + offset;
  int result;
#if defined(LIBSMM_TRANSPOSE_BLOCKDIM_MAX)
  if (LIBSMM_TRANSPOSE_BLOCKDIM_MAX >= m && LIBSMM_TRANSPOSE_BLOCKDIM_MAX >= n) {
    LIBSMM_TRANSPOSE(float, stack, nblks, m, n, dev_data);
    result = EXIT_SUCCESS;
  }
  else
#endif
  { /* TODO: well-performing library based implementation */
    result = EXIT_FAILURE;
  }
  return result;
}
#if defined(ACC_OPENMP_OFFLOAD)
# pragma omp end declare target
#endif

#if defined(__cplusplus)
}
#endif
