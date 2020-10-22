/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "../acc_libsmm.h"
#include <stdlib.h>
#if defined(__LIBXSMM)
# include <libxsmm.h>
# if !defined(SHUFFLE) && 0
#   define SHUFFLE
# endif
#endif

#if !defined(ALIGNMENT)
# define ALIGNMENT 64
#endif
#if !defined(PRIORITY)
# define PRIORITY
#endif
#if !defined(MAX_KERNEL_DIM)
# define MAX_KERNEL_DIM 80
#endif
#if !defined(ELEM_TYPE)
# define ELEM_TYPE double
#endif

#define ROUNDUP2(N, NPOT) ((((unsigned long long)N) + ((NPOT) - 1)) & ~((NPOT) - 1))
#define CHECK(EXPR, RPTR) if ((NULL != ((const void*)(RPTR)) && EXIT_SUCCESS != *((const int*)(RPTR))) || \
  EXIT_SUCCESS != (NULL != ((const void*)(RPTR)) ? (*((int*)(RPTR)) = (EXPR)) : (EXPR))) assert(0)


static void init(int seed, ELEM_TYPE* dst, int nrows, int ncols, int ld, double scale);
static void print(FILE* ostream, const char* label, const ELEM_TYPE* mat, int nrows, int ncols, int ld);

int main(int argc, char* argv[])
{
  const int nrepeat = (1 < argc ? atoi(argv[1]) : 1000), offset = 0;
  const int neven = (2 <= nrepeat ? ((nrepeat & 1/*odd*/) ? (nrepeat - 1) : nrepeat) : 2);
  const int stack_size = (2 < argc ? LIBXSMM_MAX(atoi(argv[2]), 1) : 30000);
  const int m = (3 < argc ? LIBXSMM_MAX(atoi(argv[3]), 1) : 23);
  const int n = (4 < argc ? LIBXSMM_MAX(atoi(argv[4]), 1) : m);
#if defined(ALIGNMENT) && (0 < ALIGNMENT)
  const size_t mn = ROUNDUP2(sizeof(ELEM_TYPE) * m * n, ALIGNMENT) / sizeof(ELEM_TYPE);
#else
  const size_t mn = m * n;
#endif
#if defined(SHUFFLE)
  const size_t shuffle = libxsmm_shuffle((unsigned int)stack_size);
#endif
#if defined(PRIORITY)
  int priomin, priomax;
#endif
  int *host_mem = NULL, *dev_mem = NULL;
  ELEM_TYPE *host_data = NULL, *dev_data = NULL;
  int result = EXIT_SUCCESS, r, i, j;
  void *stream = NULL;

  libxsmm_timer_tickint start;
  const double scale = 1.0 / stack_size;
  double duration;

  CHECK(acc_init(), &result);
#if defined(PRIORITY)
  CHECK(acc_stream_priority_range(&priomin, &priomax), &result);
  CHECK(acc_stream_create(&stream, "stream", (priomin + priomax) / 2), &result);
#else
  CHECK(acc_stream_create(&stream, "stream", -1/*invalid*/), &result);
#endif
  CHECK(acc_host_mem_allocate((void**)&host_data, sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
  CHECK(acc_host_mem_allocate((void**)&host_mem, sizeof(int) * stack_size, stream), &result);
  CHECK(acc_stream_sync(stream), &result);
  for (i = 0; i < stack_size; ++i) { /* initialize stack of matrices */
    init(i/*seed*/, host_data + mn * i, m, n, m/*ld*/, scale);
  }
  for (i = 0; i < stack_size; ++i) { /* initialize indexes */
#if defined(SHUFFLE)
    j = (int)(mn * ((shuffle * i) % stack_size));
#else
    j = (int)(mn * i);
#endif
    host_mem[i] = j;
  }
  CHECK(acc_dev_mem_allocate((void**)&dev_data, sizeof(ELEM_TYPE) * mn * stack_size), &result);
  CHECK(acc_memcpy_h2d(host_data, dev_data, sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
  CHECK(acc_dev_mem_allocate((void**)&dev_mem, sizeof(int) * stack_size), &result);
  CHECK(acc_memcpy_h2d(host_mem, dev_mem, sizeof(int) * stack_size, stream), &result);

  /* warmup execution and prebuild JIT kernels */
  CHECK(libsmm_acc_transpose(dev_mem, offset, stack_size, dev_data,
    dbcsr_type_real_8, m, n, MAX_KERNEL_DIM, stream), &result);
  CHECK(acc_stream_sync(stream), &result);
#if defined(__LIBXSMM)
  start = libxsmm_timer_tick();
#endif
  for (r = 0; r < neven; ++r) {
    CHECK(libsmm_acc_transpose(dev_mem, offset, stack_size, dev_data,
      dbcsr_type_real_8, m, n, MAX_KERNEL_DIM, stream), &result);
  }
#if defined(__LIBXSMM)
  CHECK(acc_stream_sync(stream), &result);
  duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
#endif
  assert(0 < neven);
  if (EXIT_SUCCESS == result) {
    unsigned int nerrors = 0;
    printf("bandwidth: %.1f GB/s\n", (sizeof(ELEM_TYPE) * m * n + sizeof(int))
      * stack_size / (duration * (1ULL << 30) / neven));
    printf("duration: %.1f ms\n", 1000.0 * duration / neven);
#if defined(__LIBXSMM)
    /* transfer result from device back to host for validation */
    CHECK(acc_memcpy_d2h(dev_data, host_data,
      sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
    CHECK(acc_stream_sync(stream), &result);
    if (EXIT_SUCCESS == result) {
      for (i = 0; i < stack_size; ++i) { /* initialize stack of matrices */
        ELEM_TYPE matrix[MAX_KERNEL_DIM*MAX_KERNEL_DIM];
        init(i/*seed*/, matrix, m, n, m/*ld*/, scale);
        libxsmm_itrans(matrix, sizeof(ELEM_TYPE), m, n, m/*ld*/);
        for (j = 0; j < (int)mn; ++j) {
          if (matrix[j] != host_data[i*mn+j]) {
            ++nerrors;
# if defined(_DEBUG)
            print(stderr, "gold = ", matrix, m, n, m);
            print(stderr, "this = ", host_data, m, n, m);
# endif
            break;
          }
        }
      }
      printf("errors: %u\n", nerrors);
      if (0 != nerrors) result = EXIT_FAILURE;
    }
#endif
  }
  CHECK(acc_host_mem_deallocate(host_data, stream), NULL);
  CHECK(acc_host_mem_deallocate(host_mem, stream), NULL);
  CHECK(acc_dev_mem_deallocate(dev_data), NULL);
  CHECK(acc_dev_mem_deallocate(dev_mem), NULL);
  CHECK(acc_stream_destroy(stream), NULL);
  if (EXIT_SUCCESS != result) {
    fprintf(stderr, "FAILED\n");
  }
  return result;
}


static void init(int seed, ELEM_TYPE* dst, int nrows, int ncols, int ld, double scale) {
  const double seed1 = scale * seed + scale;
  int i, j;
  for (i = 0; i < ncols; ++i) {
    for (j = 0; j < nrows; ++j) {
      const int k = i * ld + j;
      dst[k] = (ELEM_TYPE)(seed1 / (1.0 + k));
    }
    for (; j < ld; ++j) {
      const int k = i * ld + j;
      dst[k] = (ELEM_TYPE)(seed);
    }
  }
}


static void print(FILE* ostream, const char* label, const ELEM_TYPE* mat, int nrows, int ncols, int ld)
{
  int i, j;
  const char *const s = (NULL != label ? label : "");
  const int n = (int)strlen(s);
  for (i = 0; i < ncols; ++i) {
    if (0 < i) fprintf(ostream, "%*s", n, " "); else fprintf(ostream, "%s", s);
    for (j = 0; j < nrows; ++j) {
      fprintf(ostream, "%.2f ", mat[i*ld+j]);
    }
    fprintf(ostream, "\n");
  }
}
