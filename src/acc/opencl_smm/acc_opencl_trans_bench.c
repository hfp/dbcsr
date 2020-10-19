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

#if !defined(CACHELINE_NBYTES)
# define CACHELINE_NBYTES 64
#endif
#if !defined(MAX_KERNEL_DIM)
# define MAX_KERNEL_DIM 80
#endif
#if !defined(ELEM_TYPE)
# define ELEM_TYPE double
#endif

#define ROUNDUP2(N, NPOT) ((((unsigned long long)N) + ((NPOT) - 1)) & ~((NPOT) - 1))
#define CHECK(EXPR) if (EXIT_SUCCESS != result || EXIT_SUCCESS != (result = (EXPR))) assert(0)


static void init(int seed, ELEM_TYPE* dst, int nrows, int ncols, int ld, double scale) {
  const double seed1 = scale * seed + scale;
  int i;
  for (i = 0; i < ncols; ++i) {
    int j = 0;
    for (; j < nrows; ++j) {
      const int k = i * ld + j;
      dst[k] = (ELEM_TYPE)(seed1 / (1.0 + k));
    }
    for (; j < ld; ++j) {
      const int k = i * ld + j;
      dst[k] = (ELEM_TYPE)(seed);
    }
  }
}


int main(int argc, char* argv[])
{
  const int nrepeat = (1 < argc ? atoi(argv[1]) : 1000), offset = 0;
  const int stack_size = (2 < argc ? LIBXSMM_MAX(atoi(argv[2]), 1) : 30000);
  const int m = (3 < argc ? LIBXSMM_MAX(atoi(argv[3]), 1) : 23);
  const int n = (4 < argc ? LIBXSMM_MAX(atoi(argv[4]), 1) : m);
  const size_t mn = ROUNDUP2(sizeof(ELEM_TYPE) * m * n, CACHELINE_NBYTES) / sizeof(ELEM_TYPE);
#if defined(SHUFFLE)
  const size_t shuffle = libxsmm_shuffle((unsigned int)stack_size);
#endif
  int *host_mem = NULL, *dev_mem = NULL;
  ELEM_TYPE *host_data = NULL, *dev_data = NULL;
  int priomin, priomax, r, i, j;
  int result = EXIT_SUCCESS;
  void *stream = NULL;

  libxsmm_timer_tickint start;
  const double scale = 1.0 / stack_size;
  double duration;

  CHECK(acc_init());
  CHECK(acc_stream_priority_range(&priomin, &priomax));
  CHECK(acc_stream_create(&stream, "stream", (priomin + priomax) / 2));

  CHECK(acc_host_mem_allocate((void**)&host_data, sizeof(ELEM_TYPE) * mn * stack_size, stream));
  CHECK(acc_dev_mem_allocate((void**)&dev_data, sizeof(ELEM_TYPE) * mn * stack_size));
  CHECK(acc_stream_sync(stream));
  for (i = 0; i < stack_size; ++i) { /* initialize stack of matrices */
    init(i/*seed*/, host_data + mn * i, m, n, m/*ld*/, scale);
  }
  CHECK(acc_memcpy_h2d(host_data, dev_data, sizeof(ELEM_TYPE) * mn * stack_size, stream));

  CHECK(acc_host_mem_allocate((void**)&host_mem, sizeof(int) * stack_size, stream));
  CHECK(acc_dev_mem_allocate((void**)&dev_mem, sizeof(int) * stack_size));
  for (i = 0; i < stack_size; ++i) { /* initialize indexes */
#if defined(SHUFFLE)
    j = (int)(mn * (i * shuffle) % stack_size);
#else
    j = (int)(mn * i);
#endif
    host_mem[i] = j;
  }
  CHECK(acc_memcpy_h2d(host_mem, dev_mem, sizeof(int) * stack_size, stream));

  /* warmup execution and prebuild JIT kernels */
  CHECK(libsmm_acc_transpose(dev_mem, offset, stack_size,
    dev_data, dbcsr_type_real_8, m, n, MAX_KERNEL_DIM, stream));
#if defined(__LIBXSMM)
  start = libxsmm_timer_tick();
#endif
  for (r = 0; r < nrepeat; ++r) {
    CHECK(libsmm_acc_transpose(dev_mem, offset, stack_size,
      dev_data, dbcsr_type_real_8, m, n, MAX_KERNEL_DIM, stream));
  }
#if defined(__LIBXSMM)
  duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
#endif
  if (0 < nrepeat) printf("duration: %f ms\n", 1000.0 * duration / nrepeat);
#if defined(__LIBXSMM)
  { /* transfer result from device back to host for validation */
    unsigned int nerrors = 0;
    CHECK(acc_memcpy_d2h(dev_data, host_data, sizeof(ELEM_TYPE) * mn * stack_size, stream));
    for (i = 0; i < stack_size; ++i) { /* initialize stack of matrices */
      ELEM_TYPE matrix[MAX_KERNEL_DIM*MAX_KERNEL_DIM];
      init(i/*seed*/, matrix, m, n, m/*ld*/, scale);
      libxsmm_itrans(matrix, sizeof(ELEM_TYPE), m, n, m/*ld*/);
      for (j = 0; j < (int)mn; ++j) {
        if (matrix[j] != host_data[i*mn+j]) ++nerrors;
      }
    }
    printf("errors: %u\n", nerrors);
  }
#endif
  CHECK(acc_host_mem_deallocate(host_data, stream));
  CHECK(acc_host_mem_deallocate(host_mem, stream));
  CHECK(acc_dev_mem_deallocate(dev_data));
  CHECK(acc_dev_mem_deallocate(dev_mem));
  CHECK(acc_stream_destroy(stream));

  return result;
}
