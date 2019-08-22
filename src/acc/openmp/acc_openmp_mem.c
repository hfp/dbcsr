/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_openmp.h"
#include <stdlib.h>
#include <assert.h>

#if defined(ACC_OPENMP_VERSION) && (50 <= ACC_OPENMP_VERSION)
# define ACC_OPENMP_MEM_ALLOCATOR omp_null_allocator
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int acc_dev_mem_allocate(void** dev_mem, size_t n)
{
  int result;
#if defined(ACC_OPENMP)
  result = EXIT_FAILURE; /* TODO */
#else
  (void)(dev_mem); (void)(n); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_dev_mem_deallocate(void* dev_mem)
{
  int result;
#if defined(ACC_OPENMP)
  result = EXIT_FAILURE; /* TODO */
#else
  (void)(dev_mem); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_dev_mem_set_ptr(void** dev_mem, void* other, size_t lb)
{
  int result;
#if defined(ACC_OPENMP)
  result = EXIT_FAILURE; /* TODO */
#else
  (void)(dev_mem); (void)(other); (void)(lb); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_host_mem_allocate(void** host_mem, size_t n, acc_stream_t stream)
{
  int result;
#if !defined(ACC_OPENMP)
  (void)(host_mem); (void)(n); (void)(stream); /* unused */
#else
  if (NULL != host_mem || 0 == n) {
    if (NULL != host_mem) {
# if (50 <= ACC_OPENMP_VERSION)
      *host_mem = omp_alloc(n, ACC_OPENMP_MEM_ALLOCATOR);
# else
      *host_mem = malloc(n);
# endif
    }
    result = EXIT_SUCCESS;
  }
  else
#endif
  {
    result = EXIT_FAILURE;
  }
  return result;
}


int acc_host_mem_deallocate(void* host_mem, acc_stream_t stream)
{
  int result;
#if defined(ACC_OPENMP)
# if (50 <= ACC_OPENMP_VERSION)
  omp_free(host_mem, ACC_OPENMP_MEM_ALLOCATOR);
# else
  free(host_mem);
# endif
  result = EXIT_SUCCESS;
#else
  (void)(host_mem); (void)(stream); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_memcpy_h2d(const void* host_mem, void* dev_mem, size_t count, acc_stream_t stream)
{
  acc_openmp_depend_t in, out;
  int result = acc_openmp_stream_depend(stream, &in, &out);
#if !defined(ACC_OPENMP)
  (void)(host_mem); (void)(dev_mem); (void)(count); /* unused */
#else
# pragma omp master
  if (EXIT_SUCCESS == result) {
    /* capture current default device before spawning task (acc_set_active_device) */
    const int dev_src = omp_get_initial_device(), dev_dst = omp_get_default_device();
    acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream; assert(NULL != s);
#   pragma omp task ACC_OPENMP_DEPEND_IN(in) ACC_OPENMP_DEPEND_OUT(out)
    s->status |= omp_target_memcpy(dev_mem, (void*)host_mem, count,
      0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
  }
#endif
  return result;
}


int acc_memcpy_d2h(const void* dev_mem, void* host_mem, size_t count, acc_stream_t stream)
{
  acc_openmp_depend_t in, out;
  int result = acc_openmp_stream_depend(stream, &in, &out);
#if !defined(ACC_OPENMP)
  (void)(dev_mem); (void)(host_mem); (void)(count); /* unused */
#else
# pragma omp master
  if (EXIT_SUCCESS == result) {
    /* capture current default device before spawning task (acc_set_active_device) */
    const int dev_src = omp_get_default_device(), dev_dst = omp_get_initial_device();
    acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream; assert(NULL != s);
#   pragma omp task ACC_OPENMP_DEPEND_IN(in) ACC_OPENMP_DEPEND_OUT(out)
    s->status |= omp_target_memcpy(host_mem, (void*)dev_mem, count,
      0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
  }
#endif
  return result;
}


int acc_memcpy_d2d(const void* devmem_src, void* devmem_dst, size_t count, acc_stream_t stream)
{
  acc_openmp_depend_t in, out;
  int result = acc_openmp_stream_depend(stream, &in, &out);
#if !defined(ACC_OPENMP)
  (void)(devmem_src); (void)(devmem_dst); (void)(count); /* unused */
#else
# pragma omp master
  if (EXIT_SUCCESS == result) {
    /* capture current default device before spawning task (acc_set_active_device) */
    const int dev_src = omp_get_default_device(), dev_dst = dev_src;
    acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream; assert(NULL != s);
#   pragma omp task ACC_OPENMP_DEPEND_IN(in) ACC_OPENMP_DEPEND_OUT(out)
    s->status |= omp_target_memcpy(devmem_dst, (void*)devmem_src, count,
      0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
  }
#endif
  return result;
}


int acc_memset_zero(void* dev_mem, size_t offset, size_t length, acc_stream_t stream)
{
  int result;
#if defined(ACC_OPENMP)
  result = EXIT_FAILURE; /* TODO */
#else
  (void)(dev_mem); (void)(offset); (void)(length); (void)(stream); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_dev_mem_info(size_t* mem_free, size_t* mem_avail)
{
  int result;
#if !defined(ACC_OPENMP)
  (void)(mem_free); (void)(mem_avail); /* unused */
#else
  if (NULL != mem_free || NULL != mem_avail) {
    if (NULL != mem_free) {
    }
    if (NULL != mem_avail) {
    }
    result = EXIT_FAILURE; /* TODO */
  }
  else
#endif
  {
    result = EXIT_FAILURE;
  }
  return result;
}

#if defined(__cplusplus)
}
#endif
