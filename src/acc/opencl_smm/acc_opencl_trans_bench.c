/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_opencl_smm.h"

#if !defined(ELEM_TYPE)
# define ELEM_TYPE double
#endif
#if !defined(SHUFFLE) && 0
# define SHUFFLE
#endif


int main(int argc, char* argv[])
{
  const int stack_size = (1 < argc ? atoi(argv[1]) : 30000);
  const int m = (2 < argc ? atoi(argv[2]) : 23);
  const int n = (3 < argc ? atoi(argv[3]) : m);
  const int max_kernel_dim = 80, offset = 0;
  const size_t size = ACC_OPENCL_UP2(sizeof(ELEM_TYPE) * m * n, ACC_OPENCL_CACHELINE_NBYTES);
#if defined(SHUFFLE)
  const size_t shuffle = libxsmm_shuffle((unsigned int)stack_size);
#endif

  int *host_mem, *dev_mem;
  ELEM_TYPE *host_data, *dev_data;
  int result = EXIT_SUCCESS;
  int priomin, priomax, i;
  void *stream = NULL;

  libxsmm_timer_tickint start;
  const double scale = 1.0 / stack_size;
  double duration;

  acc_init();
  acc_stream_priority_range(&priomin, &priomax);
  acc_stream_create(&stream, "stream", (priomin + priomax) / 2);

  acc_host_mem_allocate((void**)&host_data, size * stack_size, stream);
  acc_dev_mem_allocate((void**)&dev_data, size * stack_size);
  for (i = 0; i < stack_size; ++i) { /* initialize stack of matrices */
    LIBXSMM_MATINIT(ELEM_TYPE, i/*seed*/, host_data + size * i, m, n, m/*ld*/, scale);
  }
  acc_memcpy_h2d(host_data, dev_data, size * stack_size, stream);

  acc_host_mem_allocate((void**)&host_mem, sizeof(int) * stack_size, stream);
  acc_dev_mem_allocate((void**)&dev_mem, sizeof(int) * stack_size);
  for (i = 0; i < stack_size; ++i) { /* initialize stack of matrices */
#if defined(SHUFFLE)
    const size_t j = size * (i * shuffle) % stack_size;
#else
    const size_t j = size * i;
#endif
    host_mem[i] = j;
  }
  acc_memcpy_h2d(host_mem, dev_mem, sizeof(int) * stack_size, stream);

  start = libxsmm_timer_tick();
  result = libsmm_acc_transpose((const int*)dev_mem, offset, stack_size,
    dev_data, dbcsr_type_real_8, m, n, max_kernel_dim, stream);
  duration = libxsmm_timer_duration(start, libxsmm_timer_tick());

  printf("duration: %f ms\n", 1000.0 * duration);

  acc_host_mem_deallocate(host_data, stream);
  acc_host_mem_deallocate(host_mem, stream);
  acc_dev_mem_deallocate(dev_data);
  acc_dev_mem_deallocate(dev_mem);
  acc_stream_destroy(stream);

  return result;
}
