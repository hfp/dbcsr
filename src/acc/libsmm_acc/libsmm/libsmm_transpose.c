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
#define LIBSMM_TRANSPOSE(TYPE, DEPEND_IN, DEPEND_OUT, INDEX, OFFSET, INDEXSIZE, M, N, DATA) { \
  int s; \
  ACC_OPENMP_PRAGMA(omp target teams distribute parallel for simd \
    depend(in:ACC_OPENMP_DEP(DEPEND_IN)) depend(out:ACC_OPENMP_DEP(DEPEND_OUT)) \
    nowait is_device_ptr(INDEX,DATA)) \
  for (s = 0; s < (INDEXSIZE); ++s) { \
    TYPE tmp[LIBSMM_TRANSPOSE_BLOCKDIM_MAX*LIBSMM_TRANSPOSE_BLOCKDIM_MAX]; \
    const int idx = (INDEX)[(OFFSET)+s]; \
    TYPE *const mat = &((DATA)[idx]); \
    int i, j; \
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

int libsmm_acc_transpose_d(const acc_openmp_dependency_t* in, const acc_openmp_dependency_t* out,
  const int* dev_trs_stack, int offset, int nblks, double* dev_data, int m, int n)
{
  int result;
  (void)(in); (void)(out); /* suppress incorrect warning */
#if defined(LIBSMM_TRANSPOSE_BLOCKDIM_MAX)
  if (LIBSMM_TRANSPOSE_BLOCKDIM_MAX >= m && LIBSMM_TRANSPOSE_BLOCKDIM_MAX >= n) {
    LIBSMM_TRANSPOSE(double, in, out, dev_trs_stack, nblks, m, n, dev_data);
    result = EXIT_SUCCESS;
  }
  else
#endif
  { /* TODO: well-performing library based implementation */
    result = EXIT_FAILURE;
  }
  return result;
}


int libsmm_acc_transpose_s(const acc_openmp_dependency_t* in, const acc_openmp_dependency_t* out,
  const int* dev_trs_stack, int offset, int nblks, float* dev_data, int m, int n)
{
  int result;
  (void)(in); (void)(out); /* suppress incorrect warning */
#if defined(LIBSMM_TRANSPOSE_BLOCKDIM_MAX)
  if (LIBSMM_TRANSPOSE_BLOCKDIM_MAX >= m && LIBSMM_TRANSPOSE_BLOCKDIM_MAX >= n) {
    LIBSMM_TRANSPOSE(float, in, out, dev_trs_stack, nblks, m, n, dev_data);
    result = EXIT_SUCCESS;
  }
  else
#endif
  { /* TODO: well-performing library based implementation */
    result = EXIT_FAILURE;
  }
  return result;
}

#if defined(__cplusplus)
}
#endif
