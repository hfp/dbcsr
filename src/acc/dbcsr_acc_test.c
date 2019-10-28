/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "include/acc.h"
#include <stdlib.h>
#if !defined(NDEBUG)
# include <inttypes.h>
# include <assert.h>
# include <stdio.h>
#endif
#if defined(_OPENMP)
# include <omp.h>
#endif

#if !defined(ACC_STRING_MAXLEN)
# define ACC_STRING_MAXLEN 32
#endif
#if !defined(ACC_STREAM_MAXCOUNT)
# define ACC_STREAM_MAXCOUNT 16
#endif
#if !defined(ACC_EVENT_MAXCOUNT)
# define ACC_EVENT_MAXCOUNT (16*ACC_STREAM_MAXCOUNT)
#endif
#if !defined(ACC_STREAM_MAXNTH_DESTROY)
# define ACC_STREAM_MAXNTH_DESTROY 2
#endif
#if !defined(ACC_EVENT_MAXNTH_DESTROY)
# define ACC_EVENT_MAXNTH_DESTROY 3
#endif

#if defined(NDEBUG)
# define ACC_CHECK(RESULT) do { \
    const int acc_check_result_ = (RESULT); \
    if (EXIT_SUCCESS != acc_check_result_) exit(acc_check_result_) \
  } while(0)
# define PRINTF(...)
#else /* debug */
# define ACC_CHECK(RESULT) assert(EXIT_SUCCESS == (RESULT))
# define PRINTF(...) printf(__VA_ARGS__)
#endif


/**
 * This program tests the ACC interface (include/acc.h) for adhering to expectations.
 * The expected behavior is to match the CUDA based implementation, which was available
 * first. This test program can serve as a specification for other backends such as the
 * OpenMP based backend. It may also be used to stress-test any backend including the
 * CUDA based backend for thread-safety. Thread-safety is an implicit requirement
 * induced by DBCSR (and CP2K). To test any backend (other than the OpenMP backend),
 * the Makefile must be adjusted to link with the desired backend.
 */
int main(int argc, char* argv[])
{
  const int device = (1 < argc ? atoi(argv[1]) : 0);
  acc_stream_t stream[ACC_STREAM_MAXCOUNT];
  acc_event_t event[ACC_EVENT_MAXCOUNT];
  int priority[ACC_STREAM_MAXCOUNT];
  int randnums[ACC_EVENT_MAXCOUNT];
  int ndevices, priomin, priomax, prange, i;
  size_t mem_free, mem_total;

  for (i = 0; i < ACC_EVENT_MAXCOUNT; ++i) {
    randnums[i] = rand();
  }

  ACC_CHECK(acc_init());
  ACC_CHECK(acc_get_ndevices(&ndevices));
  PRINTF("ndevices: %i\n", ndevices);
  /* continue tests even with no device */
  if (0 <= device && device < ndevices) { /* not an error */
    ACC_CHECK(acc_set_active_device(device));
  }

  ACC_CHECK(acc_dev_mem_info(&mem_free, &mem_total));
  ACC_CHECK(mem_free <= mem_total ? EXIT_SUCCESS : EXIT_FAILURE);
  PRINTF("device memory: free=%" PRIuPTR " total=%" PRIuPTR "\n",
    (uintptr_t)mem_free, (uintptr_t)mem_total);

  ACC_CHECK(acc_stream_priority_range(&priomin, &priomax));
  prange = 1 + priomax - priomin;
  ACC_CHECK(0 < prange ? EXIT_SUCCESS : EXIT_FAILURE);
  PRINTF("stream priority: min=%i max=%i\n", priomin, priomax);

  for (i = 0; i < ACC_STREAM_MAXCOUNT; ++i) {
    priority[i] = priomin + (randnums[i%ACC_STREAM_MAXCOUNT] % prange);
    stream[i] = NULL;
  }
  for (i = 0; i < ACC_EVENT_MAXCOUNT; ++i) {
    event[i] = NULL;
  }

  ACC_CHECK(acc_stream_destroy(NULL));
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < ACC_STREAM_MAXCOUNT; ++i) {
    const int r = randnums[i%ACC_STREAM_MAXCOUNT] % ACC_STREAM_MAXCOUNT;
    char name[ACC_STRING_MAXLEN]; /* thread-local */
    const int n = sprintf(name, "%i", i);
    ACC_CHECK((0 <= n && n < ACC_STRING_MAXLEN) ? EXIT_SUCCESS : EXIT_FAILURE);
    ACC_CHECK(acc_stream_create(stream + i, name, priority[i]));
    if (ACC_STREAM_MAXNTH_DESTROY * r < ACC_STREAM_MAXCOUNT) {
      ACC_CHECK(acc_stream_destroy(stream[i]));
      stream[i] = NULL;
    }
  }

#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < ACC_STREAM_MAXCOUNT; ++i) {
    ACC_CHECK(acc_stream_destroy(stream[i]));
  }

  ACC_CHECK(acc_event_destroy(NULL));
#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < ACC_EVENT_MAXCOUNT; ++i) {
    const int r = randnums[i%ACC_EVENT_MAXCOUNT] % ACC_EVENT_MAXCOUNT;
    ACC_CHECK(acc_event_create(event + i));
    if (ACC_EVENT_MAXNTH_DESTROY * r < ACC_EVENT_MAXCOUNT) {
      ACC_CHECK(acc_event_destroy(event[i]));
      event[i] = NULL;
    }
  }

#if defined(_OPENMP)
# pragma omp parallel for private(i)
#endif
  for (i = 0; i < ACC_EVENT_MAXCOUNT; ++i) {
    ACC_CHECK(acc_event_destroy(event[i]));
  }

  ACC_CHECK(acc_clear_errors());
  ACC_CHECK(acc_finalize());

  return EXIT_SUCCESS;
}
