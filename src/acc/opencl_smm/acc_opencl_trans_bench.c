/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_opencl_smm.h"


int main(int argc, char* argv[])
{
  typedef double T;
  const int stack_size = (1 < argc ? atoi(argv[1]) : 30000);
  const int m = (2 < argc ? atoi(argv[2]) : 23);
  const int n = (3 < argc ? atoi(argv[3]) : m);
  const int max_kernel_dim = 80, offset = 0;

  void *host_mem, *dev_mem, *host_data, *dev_data;
  int result = EXIT_SUCCESS;
  int priomin, priomax;
  void *stream = NULL;

  libxsmm_timer_tickint start;
  double duration;

  acc_init();
  acc_stream_priority_range(&priomin, &priomax);
  acc_stream_create(&stream, "stream", (priomin + priomax) / 2);

  acc_host_mem_allocate(&host_mem, stack_size * sizeof(int), stream);
  acc_dev_mem_allocate(&dev_mem, stack_size * sizeof(int));

  /* TODO: initialize matrix stack */

  acc_host_mem_allocate(&host_data, stack_size * m * n * sizeof(T), stream);
  acc_dev_mem_allocate(&dev_data, stack_size * m * n * sizeof(T));

  /* TODO: initialize matrices */

  acc_memcpy_h2d(host_mem, dev_mem, stack_size * sizeof(int), stream);
  acc_memcpy_h2d(host_data, dev_data, stack_size * m * n * sizeof(T), stream);

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
