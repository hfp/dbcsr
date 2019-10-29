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
#include <string.h>

#if defined(_WIN32)
# include <Windows.h>
#else
# include <unistd.h>
#endif

#if defined(ACC_OPENMP_VERSION) && (50 <= ACC_OPENMP_VERSION)
# define ACC_OPENMP_MEM_ALLOCATOR omp_null_allocator
#endif

#if defined(__cplusplus)
extern "C" {
#endif


int acc_host_mem_allocate(void** host_mem, size_t n, acc_stream_t stream)
{
  int result;
  /* Allocation is not enqueued because other function signature
   * taking host memory expect a pointer rather than a pointer
   * to a pointer, which would allow for asynchronous delivery.
   */
  (void)(stream); /* unused */
  assert(NULL != host_mem || 0 == n);
#if defined(ACC_OPENMP)
# pragma omp single
#endif
  if (0 != n) {
#if (50 <= ACC_OPENMP_VERSION)
    *host_mem = omp_alloc(n, ACC_OPENMP_MEM_ALLOCATOR);
#else
    *host_mem = malloc(n);
#endif
    result = ((NULL != *host_mem || 0 == n) ? EXIT_SUCCESS : EXIT_FAILURE);
  }
  else result = EXIT_SUCCESS;
  return result;
}


int acc_host_mem_deallocate(void* host_mem, acc_stream_t stream)
{
  int result;
#if defined(ACC_OPENMP)
# pragma omp master
#endif
  if (NULL != host_mem) {
    acc_openmp_depend_t in, out;
    result = acc_openmp_stream_depend(stream, &in, &out);
    if (EXIT_SUCCESS == result) {
      /* freeing memory must be in order (enqueued) */
#     pragma omp task ACC_OPENMP_DEPEND_IN(in) ACC_OPENMP_DEPEND_OUT(out)
#if (50 <= ACC_OPENMP_VERSION)
      omp_free(host_mem, ACC_OPENMP_MEM_ALLOCATOR);
#else
      free(host_mem);
#endif
    }
  }
  else result = EXIT_SUCCESS;
  return result;
}


int acc_dev_mem_allocate(void** dev_mem, size_t n)
{
  assert(NULL != dev_mem);
#if defined(ACC_OPENMP)
  if (0 < omp_get_num_devices()) {
    *dev_mem = omp_target_alloc(n, omp_get_default_device());
  }
  else
#endif
  {
    *dev_mem = malloc(n);
  }
  return (NULL != *dev_mem ? EXIT_SUCCESS : EXIT_FAILURE);
}


int acc_dev_mem_deallocate(void* dev_mem)
{
  if (NULL != dev_mem) {
#if defined(ACC_OPENMP)
    if (0 < omp_get_num_devices()) {
      omp_target_free(dev_mem, omp_get_default_device());
    }
    else
#endif
    {
      free(dev_mem);
    }
  }
  return EXIT_SUCCESS;
}


int acc_dev_mem_set_ptr(void** dev_mem, void* other, size_t lb)
{
  int result;
  assert(NULL != dev_mem);
  if (NULL != other) {
    /* OpenMP specification: pointer arithmetic may not be valid */
    *dev_mem = (char*)other + lb;
    result = EXIT_SUCCESS;
  }
  else result = EXIT_FAILURE;
  return result;
}


int acc_memcpy_h2d(const void* host_mem, void* dev_mem, size_t count, acc_stream_t stream)
{
  int result;
  assert((NULL != host_mem && NULL != dev_mem) || 0 == count);
#if defined(ACC_OPENMP)
# pragma omp master
#endif
  if (0 != count) {
    acc_openmp_depend_t in, out;
    result = acc_openmp_stream_depend(stream, &in, &out);
    if (EXIT_SUCCESS == result) {
#if defined(ACC_OPENMP)
      /* capture current default device before spawning task (acc_set_active_device) */
      const int dev_src = omp_get_initial_device(), dev_dst = omp_get_default_device();
      acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream; assert(NULL != s);
#     pragma omp task ACC_OPENMP_DEPEND_IN(in) ACC_OPENMP_DEPEND_OUT(out)
      s->status |= omp_target_memcpy(dev_mem, (void*)host_mem, count,
        0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
#else
      memcpy(dev_mem, host_mem, count);
#endif
    }
  }
  else result = EXIT_SUCCESS;
  return result;
}


int acc_memcpy_d2h(const void* dev_mem, void* host_mem, size_t count, acc_stream_t stream)
{
  int result;
  assert((NULL != dev_mem && NULL != host_mem) || 0 == count);
#if defined(ACC_OPENMP)
# pragma omp master
#endif
  if (0 != count) {
    acc_openmp_depend_t in, out;
    result = acc_openmp_stream_depend(stream, &in, &out);
    if (EXIT_SUCCESS == result) {
#if defined(ACC_OPENMP)
      /* capture current default device before spawning task (acc_set_active_device) */
      const int dev_src = omp_get_default_device(), dev_dst = omp_get_initial_device();
      acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream; assert(NULL != s);
#     pragma omp task ACC_OPENMP_DEPEND_IN(in) ACC_OPENMP_DEPEND_OUT(out)
      s->status |= omp_target_memcpy(host_mem, (void*)dev_mem, count,
        0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
#else
      memcpy(host_mem, dev_mem, count);
#endif
    }
  }
  else result = EXIT_SUCCESS;
  return result;
}


int acc_memcpy_d2d(const void* devmem_src, void* devmem_dst, size_t count, acc_stream_t stream)
{
  int result;
  assert((NULL != devmem_src && NULL != devmem_dst) || 0 == count);
#if defined(ACC_OPENMP)
# pragma omp master
#endif
  if (0 != count) {
    acc_openmp_depend_t in, out;
    result = acc_openmp_stream_depend(stream, &in, &out);
    if (EXIT_SUCCESS == result) {
#if defined(ACC_OPENMP)
      /* capture current default device before spawning task (acc_set_active_device) */
      const int dev_src = omp_get_default_device(), dev_dst = dev_src;
      acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream; assert(NULL != s);
#     pragma omp task ACC_OPENMP_DEPEND_IN(in) ACC_OPENMP_DEPEND_OUT(out)
      s->status |= omp_target_memcpy(devmem_dst, (void*)devmem_src, count,
        0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
#else
      memcpy(devmem_dst, devmem_src, count);
#endif
    }
  }
  else result = EXIT_SUCCESS;
  return result;
}


int acc_memset_zero(void* dev_mem, size_t offset, size_t length, acc_stream_t stream)
{
  int result;
  assert(NULL != dev_mem || 0 == length);
#if defined(ACC_OPENMP)
# pragma omp master
#endif
  if (0 != length) {
    acc_openmp_depend_t in, out;
    result = acc_openmp_stream_depend(stream, &in, &out);
    if (EXIT_SUCCESS == result) {
      /* OpenMP specification: pointer arithmetic may not be valid */
      char *const dst = (char*)dev_mem + offset;
      size_t i;
#if defined(ACC_OPENMP)
#     pragma omp target teams distribute parallel for simd /*private(i)*/ \
      ACC_OPENMP_DEPEND_IN(in) ACC_OPENMP_DEPEND_OUT(out) nowait is_device_ptr(dst)
#endif
      for (i = 0; i < length; ++i) dst[i] = '\0';
    }
  }
  else result = EXIT_SUCCESS;
  return result;
}


int acc_dev_mem_info(size_t* mem_free, size_t* mem_total)
{
  int ndevices, result = (NULL != mem_free || NULL != mem_total)
    ? acc_get_ndevices(&ndevices) : EXIT_FAILURE;
  if (EXIT_SUCCESS == result) {
    size_t size_free = 0, size_total = 0;
    /* There is no OpenMP function that returns memory statistics of a device.
     * Instead, the free/available host memory divided by the number of devices
     * is used as a proxy value.
     */
    if (0 < ndevices) {
#if defined(_WIN32)
      MEMORYSTATUSEX mem_status;
      mem_status.dwLength = sizeof(mem_status);
      if (GlobalMemoryStatusEx(&mem_status)) {
        size_total = (size_t)mem_status.ullTotalPhys;
        size_free = (size_t)mem_status.ullAvailPhys;
      }
      else result = EXIT_FAILURE;
#else
# if defined(_SC_PAGE_SIZE)
      const long page_size = sysconf(_SC_PAGE_SIZE);
# else
      const long page_size = 4096;
# endif
# if defined(_SC_AVPHYS_PAGES)
      const long pages_free = sysconf(_SC_AVPHYS_PAGES);
# else
      const long pages_free = 0;
# endif
# if defined(_SC_PHYS_PAGES)
      const long pages_total = sysconf(_SC_PHYS_PAGES);
# else
      const long pages_total = pages_free;
# endif
      if (0 < page_size && 0 <= pages_free && 0 <= pages_total) {
        const size_t size_page = (size_t)page_size;
        size_total = (size_page * (size_t)pages_total);
        size_free = (size_page * (size_t)pages_free);
      }
      else result = EXIT_FAILURE;
#endif
      if (EXIT_SUCCESS == result) {
        size_total /= ndevices;
        size_free /= ndevices;
      }
    }
    if (size_free <= size_total) { /* EXIT_SUCCESS != result is ok */
      if (NULL != mem_total) *mem_total = size_total;
      if (NULL != mem_free) *mem_free = size_free;
    }
    else if (EXIT_SUCCESS == result) {
      result = EXIT_FAILURE;
    }
  }
  return result;
}

#if defined(__cplusplus)
}
#endif
