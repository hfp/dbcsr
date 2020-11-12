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
#include <assert.h>
#include <stdio.h>

#if defined(__LIBXSMM)
# include <libxsmm.h>
# if !defined(SHUFFLE) && 0
#   define SHUFFLE
# endif
#endif

#if !defined(ELEM_TYPE)
# define ELEM_TYPE double
#endif
#if !defined(MAX_KERNEL_DIM)
# define MAX_KERNEL_DIM 80
#endif
#if !defined(ALIGNMENT)
# define ALIGNMENT 64
#endif
#if !defined(PRIORITY)
# define PRIORITY
#endif
#if !defined(WARMUP)
# define WARMUP
#endif

#define MAX(a, b) ((b) < (a) ? (a) : (b))
#define ROUNDUP2(N, NPOT) ((((unsigned long long)N) + ((NPOT) - 1)) & ~((NPOT) - 1))
#define CHECK(EXPR, RPTR) if ((NULL != ((const void*)(RPTR)) && EXIT_SUCCESS != *((const int*)(RPTR))) || \
  EXIT_SUCCESS != (NULL != ((const void*)(RPTR)) ? (*((int*)(RPTR)) = (EXPR)) : (EXPR))) assert(0)


#if defined(_DEBUG)
static void print(FILE* ostream, const char* label, const ELEM_TYPE* mat, int m, int n, int ld);
#endif

static void init(int seed, ELEM_TYPE* dst, int m, int n, int ld, double scale);
static void swap(int* m, int* n) { int tmp = *m; *m = *n; *n = tmp; }


int main(int argc, char* argv[])
{
  const int nrepeat = (1 < argc ? atoi(argv[1]) : 5), offset = 0;
  const int nodd = (0 < nrepeat ? ((nrepeat & 1/*odd*/) ? nrepeat : (nrepeat - 1)) : 1);
  const int stack_size = (2 < argc ? atoi(argv[2]) : 30000);
  const int m = (3 < argc ? atoi(argv[3]) : 23);
  const int n = (4 < argc ? atoi(argv[4]) : m);
  const int ld = MAX(m, n);
#if defined(ALIGNMENT) && (0 < ALIGNMENT)
  const size_t mn = ROUNDUP2(sizeof(ELEM_TYPE) * ld * ld, ALIGNMENT) / sizeof(ELEM_TYPE);
#else
  const size_t mn = ld * ld;
#endif
#if defined(SHUFFLE)
  const size_t shuffle = libxsmm_shuffle((unsigned int)stack_size);
#endif
#if defined(PRIORITY)
  int priomin, priomax;
#endif
  int *host_mem = NULL, *dev_mem = NULL;
  ELEM_TYPE *host_data = NULL, *dev_data = NULL;
  int result = EXIT_SUCCESS, r, i, j, mm = m, nn = n;
  void *stream = NULL;
#if defined(__LIBXSMM)
  libxsmm_timer_tickint start;
  double duration;
#endif

  printf("%s%s%i %i %i %i\n", 0 < argc ? argv[0] : "", 0 < argc ? " " : "", nrepeat, stack_size, m, n);
  CHECK(acc_init(), &result);
#if defined(PRIORITY)
  CHECK(acc_stream_priority_range(&priomin, &priomax), &result);
  CHECK(acc_stream_create(&stream, "stream", (priomin + priomax) / 2), &result);
#else
  CHECK(acc_stream_create(&stream, "stream", -1/*invalid*/), &result);
#endif
  CHECK(acc_host_mem_allocate((void**)&host_data, sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
  CHECK(acc_host_mem_allocate((void**)&host_mem, sizeof(int) * stack_size, stream), &result);
  CHECK(acc_stream_sync(stream), &result); /* ensure host-data is allocated */
  assert((size_t)ld <= mn / n);
  for (i = 0; i < stack_size; ++i) { /* initialize stack of matrices */
    init(i/*seed*/, host_data + mn * i, m, n, mn / n/*ld*/, 1.0/*scale*/);
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
  CHECK(acc_dev_mem_allocate((void**)&dev_mem, sizeof(int) * stack_size), &result);
#if defined(__LIBXSMM)
  CHECK(acc_stream_sync(stream), &result);
  start = libxsmm_timer_tick();
#endif
  CHECK(acc_memcpy_h2d(host_data, dev_data, sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
  CHECK(acc_memcpy_h2d(host_mem, dev_mem, sizeof(int) * stack_size, stream), &result);
#if defined(__LIBXSMM)
  CHECK(acc_stream_sync(stream), &result);
  duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
  printf("copy-in: %.1f ms %.1f GB/s\n", 1000.0 * duration,
    (sizeof(ELEM_TYPE) * m * n + sizeof(int))
      * stack_size / (duration * (1ULL << 30)));
#endif
#if defined(WARMUP)
  /* warmup execution and prebuild JIT kernels */
  CHECK(libsmm_acc_transpose(dev_mem, offset, stack_size, dev_data,
    dbcsr_type_real_8, m, n, MAX_KERNEL_DIM, stream), &result);
  CHECK(libsmm_acc_transpose(dev_mem, offset, stack_size, dev_data,
    dbcsr_type_real_8, n, m, MAX_KERNEL_DIM, stream), &result);
#endif
#if defined(__LIBXSMM)
  CHECK(acc_stream_sync(stream), &result);
  start = libxsmm_timer_tick();
#endif
  for (r = 0; r < nodd; ++r) {
    CHECK(libsmm_acc_transpose(dev_mem, offset, stack_size, dev_data,
      dbcsr_type_real_8, mm, nn, MAX_KERNEL_DIM, stream), &result);
    swap(&mm, &nn);
  }
#if defined(__LIBXSMM)
  CHECK(acc_stream_sync(stream), &result);
  duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
  if (EXIT_SUCCESS == result) {
    assert(0 < nodd && (nodd & 1/*odd*/));
    printf("device: %.1f ms %.1f GB/s\n", 1000.0 * duration / nodd,
      (sizeof(ELEM_TYPE) * m * n + sizeof(int))
        * stack_size / (duration * (1ULL << 30) / nodd));
    mm = m; nn = n;
    start = libxsmm_timer_tick();
    for (r = 0; r < nodd; ++r) {
      libxsmm_itrans_batch_omp(host_data, sizeof(ELEM_TYPE), mm, nn, ld,
        0/*index_base*/, sizeof(int)/*index_stride*/, host_mem, stack_size);
      swap(&mm, &nn);
    }
    duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
    printf("host: %.1f ms %.1f GB/s\n", 1000.0 * duration / nodd,
      (sizeof(ELEM_TYPE) * m * n + sizeof(int))
        * stack_size / (duration * (1ULL << 30) / nodd));
    /* transfer result from device back to host for validation */
    CHECK(acc_memcpy_d2h(dev_data, host_data,
      sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
    CHECK(acc_stream_sync(stream), &result);
    if (EXIT_SUCCESS == result) {
      unsigned int nerrors = 0;
      for (i = 0; i < stack_size; ++i) {
        ELEM_TYPE gold[MAX_KERNEL_DIM*MAX_KERNEL_DIM];
        const ELEM_TYPE *const test = host_data + mn * i;
        init(i/*seed*/, gold, m, n, ld, 1.0/*scale*/);
        libxsmm_itrans(gold, sizeof(ELEM_TYPE), m, n, ld);
        for (j = 0; j < (ld * n); ++j) {
          if (gold[j] != test[j]) {
            ++nerrors;
# if defined(_DEBUG)
            print(stderr, "gold = ", gold, n, m, ld);
            print(stderr, "this = ", test, n, m, ld);
            fprintf(stderr, "\n");
# endif
            break;
          }
        }
      }
      printf("errors: %u\n", nerrors);
      if (0 != nerrors) result = EXIT_FAILURE;
    }
  }
#endif
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


static void init(int seed, ELEM_TYPE* dst, int m, int n, int ld, double scale) {
  const double seed1 = scale * seed + scale;
  int i, j;
  for (i = 0; i < n; ++i) {
    for (j = 0; j < m; ++j) {
      const int k = i * ld + j;
      dst[k] = (ELEM_TYPE)(seed1 * (1.0 + i * m + j));
    }
    for (; j < ld; ++j) {
      const int k = i * ld + j;
      dst[k] = (ELEM_TYPE)(seed);
    }
  }
}


#if defined(_DEBUG)
static void print(FILE* ostream, const char* label, const ELEM_TYPE* mat, int m, int n, int ld)
{
  int i, j;
  const char *const s = (NULL != label ? label : "");
  const int len = (int)strlen(s);
  for (i = 0; i < n; ++i) {
    if (0 < i) fprintf(ostream, "%*s", len, " "); else fprintf(ostream, "%s", s);
    for (j = 0; j < m; ++j) {
      fprintf(ostream, "%.2f ", mat[i*ld+j]);
    }
    fprintf(ostream, "\n");
  }
}
#endif
