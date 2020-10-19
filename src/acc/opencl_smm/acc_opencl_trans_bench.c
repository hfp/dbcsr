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
  const int stack_size = (1 < argc ? LIBXSMM_MAX(atoi(argv[1]), 1) : 30000);
  const int m = (2 < argc ? LIBXSMM_MAX(atoi(argv[2]), 1) : 23);
  const int n = (3 < argc ? LIBXSMM_MAX(atoi(argv[3]), 1) : m);
  const int max_kernel_dim = 80, offset = 0;
  const size_t mn = ROUNDUP2(sizeof(ELEM_TYPE) * m * n, CACHELINE_NBYTES) / sizeof(ELEM_TYPE);
#if defined(SHUFFLE)
  const size_t shuffle = libxsmm_shuffle((unsigned int)stack_size);
#endif
  int *host_mem = NULL, *dev_mem = NULL;
  ELEM_TYPE *host_data = NULL, *dev_data = NULL;
  int result = EXIT_SUCCESS;
  int priomin, priomax, i;
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
    const size_t j = mn * (i * shuffle) % stack_size;
#else
    const size_t j = mn * i;
#endif
    host_mem[i] = j;
  }
  CHECK(acc_memcpy_h2d(host_mem, dev_mem, sizeof(int) * stack_size, stream));
#if defined(__LIBXSMM)
  start = libxsmm_timer_tick();
#endif
  CHECK(libsmm_acc_transpose((const int*)dev_mem, offset, stack_size,
    dev_data, dbcsr_type_real_8, m, n, max_kernel_dim, stream));
#if defined(__LIBXSMM)
  duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
#endif
  printf("duration: %f ms\n", 1000.0 * duration);

  CHECK(acc_host_mem_deallocate(host_data, stream));
  CHECK(acc_host_mem_deallocate(host_mem, stream));
  CHECK(acc_dev_mem_deallocate(dev_data));
  CHECK(acc_dev_mem_deallocate(dev_mem));
  CHECK(acc_stream_destroy(stream));

  return result;
}
