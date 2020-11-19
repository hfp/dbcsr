/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_libsmm.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#if defined(__LIBXSMM)
# include <libxsmm.h>
# define USE_LIBXSMM
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
#if !defined(WARMUP)
# define WARMUP 2
#endif

#define MAX(a, b) ((b) < (a) ? (a) : (b))
#define ROUNDUP2(N, NPOT) ((((unsigned long long)N) + ((NPOT) - 1)) & ~((NPOT) - 1))
#define CHECK(EXPR, RPTR) if ((NULL != ((const void*)(RPTR)) && EXIT_SUCCESS != *((const int*)(RPTR))) || \
  EXIT_SUCCESS != (NULL != ((const void*)(RPTR)) ? (*((int*)(RPTR)) = (EXPR)) : (EXPR))) assert(0)


#if defined(_DEBUG) && defined(USE_LIBXSMM)
static void print(FILE* ostream, const char* label, const ELEM_TYPE* mat, int m, int n);
#endif

static void init(int seed, ELEM_TYPE* dst, int m, int n);


int main(int argc, char* argv[])
{
  const int nrepeat = (1 < argc ? atoi(argv[1]) : 5);
  const int stack_size = (2 < argc ? atoi(argv[2]) : 30000);
  const int m = (3 < argc ? atoi(argv[3]) : 23);
  const int n = (4 < argc ? atoi(argv[4]) : m);
  const int k = (5 < argc ? atoi(argv[5]) : m);
#if defined(ALIGNMENT) && (0 < ALIGNMENT)
  const size_t ma = ROUNDUP2(sizeof(ELEM_TYPE) * m, ALIGNMENT);
  const size_t ka = ROUNDUP2(sizeof(ELEM_TYPE) * k, ALIGNMENT);
  const size_t mn = ma * n / sizeof(ELEM_TYPE);
  const size_t mk = ma * k / sizeof(ELEM_TYPE);
  const size_t kn = ka * n / sizeof(ELEM_TYPE);
#else
  const size_t mn = m * n, mk = m * k, kn = k * n;
#endif
#if defined(SHUFFLE)
  const size_t shuffle = libxsmm_shuffle((unsigned int)stack_size);
#endif
#if defined(WARMUP) && (0 < WARMUP) && !defined(_DEBUG)
  const int warmup = WARMUP;
#else
  const int warmup = 0;
#endif
  libsmm_acc_smmstack_host_t *stack_hst = NULL, *stack_dev = NULL;
  ELEM_TYPE *amat_hst = NULL, *bmat_hst = NULL, *cmat_hst = NULL;
  ELEM_TYPE *amat_dev = NULL, *bmat_dev = NULL, *cmat_dev = NULL;
  int result = EXIT_SUCCESS, r, i, j;
  void *stream = NULL;
#if defined(USE_LIBXSMM)
  libxsmm_timer_tickint start;
  double duration;
#endif
  assert(m <= (int)(mn / n) && 0 == (mn % n) && k <= (int)(mk / k) && 0 == (mk % k) && n <= (int)(kn / n) && 0 == (kn % n));
  printf("%s%s%i %i %i %i %i\n", 0 < argc ? argv[0] : "", 0 < argc ? " " : "", nrepeat, stack_size, m, n, k);
  CHECK(acc_init(), &result);
  CHECK(acc_stream_create(&stream, "stream", -1/*default priority*/), &result);
  CHECK(acc_host_mem_allocate((void**)&amat_hst, sizeof(ELEM_TYPE) * mk * stack_size, stream), &result);
  CHECK(acc_host_mem_allocate((void**)&bmat_hst, sizeof(ELEM_TYPE) * kn * stack_size, stream), &result);
  CHECK(acc_host_mem_allocate((void**)&cmat_hst, sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
  CHECK(acc_host_mem_allocate((void**)&stack_hst, sizeof(libsmm_acc_smmstack_t) * stack_size, stream), &result);
  CHECK(acc_stream_sync(stream), &result); /* ensure host-data is allocated */
  for (i = 0; i < stack_size; ++i) { /* initialize stack of matrices */
    init(i/*seed*/, amat_hst + mn * i, m, n);
  }
  for (i = 0; i < stack_size; ++i) { /* initialize indexes */
#if defined(SHUFFLE)
    j = (int)(mn * ((shuffle * i) % stack_size));
#else
    j = (int)(mn * i);
#endif
#if 0/*TODO*/
    stack_hst[i] = j;
#endif
  }
  CHECK(acc_dev_mem_allocate((void**)&amat_dev, sizeof(ELEM_TYPE) * mk * stack_size), &result);
  CHECK(acc_dev_mem_allocate((void**)&bmat_dev, sizeof(ELEM_TYPE) * kn * stack_size), &result);
  CHECK(acc_dev_mem_allocate((void**)&cmat_dev, sizeof(ELEM_TYPE) * mn * stack_size), &result);
  CHECK(acc_dev_mem_allocate((void**)&stack_dev, sizeof(libsmm_acc_smmstack_t) * stack_size), &result);
#if defined(USE_LIBXSMM)
  CHECK(acc_stream_sync(stream), &result);
  start = libxsmm_timer_tick();
#endif
  CHECK(acc_memcpy_h2d(amat_hst, amat_dev, sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
  CHECK(acc_memcpy_h2d(bmat_hst, bmat_dev, sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
  CHECK(acc_memcpy_h2d(cmat_hst, cmat_dev, sizeof(ELEM_TYPE) * mn * stack_size, stream), &result);
  CHECK(acc_memcpy_h2d(stack_hst, stack_dev, sizeof(libsmm_acc_smmstack_t) * stack_size, stream), &result);
#if defined(USE_LIBXSMM)
  CHECK(acc_stream_sync(stream), &result);
  duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
  printf("copy-in: %.1f ms %.1f GB/s\n", 1000.0 * duration,
    (sizeof(ELEM_TYPE) * (mk + kn + mn) + sizeof(libsmm_acc_smmstack_t))
      * stack_size / (duration * (1ULL << 30)));
#endif
  /* warmup execution and prebuild JIT kernels */
  for (r = 0; r < warmup; ++r) {
#if 0 /*TODO*/
    CHECK(libsmm_acc_transpose(stack_dev, offset, stack_size, data_dev,
      dbcsr_type_real_8, m, n, MAX_KERNEL_DIM, stream), &result);
    CHECK(libsmm_acc_transpose(stack_dev, offset, stack_size, data_dev,
      dbcsr_type_real_8, n, m, MAX_KERNEL_DIM, stream), &result);
#endif
  }
#if defined(USE_LIBXSMM)
  CHECK(acc_stream_sync(stream), &result);
  start = libxsmm_timer_tick();
#endif
  for (r = 0; r < nrepeat; ++r) {
#if 0 /*TODO*/
    CHECK(libsmm_acc_transpose(stack_dev, offset, stack_size, data_dev,
      dbcsr_type_real_8, m, n, MAX_KERNEL_DIM, stream), &result);
#endif
  }
#if defined(USE_LIBXSMM) && 0
  CHECK(acc_stream_sync(stream), &result);
  duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
  if (EXIT_SUCCESS == result) {
    printf("device: %.1f ms %.1f GB/s\n", 1000.0 * duration / nrepeat,
      (sizeof(ELEM_TYPE) * (mk + kn + mn) + sizeof(libsmm_acc_smmstack_t))
        * stack_size / (duration * (1ULL << 30) / nrepeat));
    start = libxsmm_timer_tick();
    for (r = 0; r < nrepeat; ++r) {
      libxsmm_itrans_batch_omp(amat_hst, sizeof(ELEM_TYPE), m, n, m, n,
        0/*index_base*/, sizeof(libsmm_acc_smmstack_t)/*index_stride*/, stack_hst, stack_size);
    }
    duration = libxsmm_timer_duration(start, libxsmm_timer_tick());
    printf("host: %.1f ms %.1f GB/s\n", 1000.0 * duration / nrepeat,
      (sizeof(ELEM_TYPE) * (mk + kn + mn) + sizeof(libsmm_acc_smmstack_t))
        * stack_size / (duration * (1ULL << 30) / nrepeat));
    /* transfer result from device back to host for validation */
    CHECK(acc_memcpy_d2h(data_dev, amat_hst,
      sizeof(ELEM_TYPE) * (mk + kn + mn) * stack_size, stream), &result);
    CHECK(acc_stream_sync(stream), &result);
    if (EXIT_SUCCESS == result) {
      unsigned int nerrors = 0;
      for (i = 0; i < stack_size; ++i) {
        ELEM_TYPE gold[MAX_KERNEL_DIM*MAX_KERNEL_DIM];
        const ELEM_TYPE *const test = amat_hst + mn * i;
        init(i/*seed*/, gold, m, n);
        libxsmm_itrans(gold, sizeof(ELEM_TYPE), m, n, m, n);
        for (j = 0; j < (m * n); ++j) {
          if (gold[j] != test[j]) {
            ++nerrors;
# if defined(_DEBUG)
            print(stderr, "gold = ", gold, n, m);
            print(stderr, "this = ", test, n, m);
            init(i/*seed*/, gold, m, n);
            print(stderr, "orig = ", gold, m, n);
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
  CHECK(acc_host_mem_deallocate(stack_hst, stream), NULL);
  CHECK(acc_host_mem_deallocate(amat_hst, stream), NULL);
  CHECK(acc_host_mem_deallocate(bmat_hst, stream), NULL);
  CHECK(acc_host_mem_deallocate(cmat_hst, stream), NULL);
  CHECK(acc_dev_mem_deallocate(stack_dev), NULL);
  CHECK(acc_dev_mem_deallocate(amat_dev), NULL);
  CHECK(acc_dev_mem_deallocate(bmat_dev), NULL);
  CHECK(acc_dev_mem_deallocate(cmat_dev), NULL);
  CHECK(acc_stream_destroy(stream), NULL);
  if (EXIT_SUCCESS != result) {
    fprintf(stderr, "FAILED\n");
  }
  return result;
}


static void init(int seed, ELEM_TYPE* dst, int m, int n) {
  int i, j;
  for (i = 0; i < n; ++i) {
    for (j = 0; j < m; ++j) {
      const int k = i * m + j;
      dst[k] = (ELEM_TYPE)((seed + 1) * (k + 1));
    }
  }
}


#if defined(_DEBUG) && defined(USE_LIBXSMM)
static void print(FILE* ostream, const char* label, const ELEM_TYPE* mat, int m, int n)
{
  int i, j;
  const char *const s = (NULL != label ? label : "");
  const int len = (int)strlen(s);
  for (i = 0; i < n; ++i) {
    if (0 < i) fprintf(ostream, "%*s", len, " "); else fprintf(ostream, "%s", s);
    for (j = 0; j < m; ++j) {
      fprintf(ostream, "%.2f ", mat[i*m+j]);
    }
    fprintf(ostream, "\n");
  }
}
#endif
