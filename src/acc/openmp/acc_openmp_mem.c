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
# define ACC_OPENMP_MEM_ALLOC(SIZE) omp_alloc(SIZE, omp_null_allocator)
# define ACC_OPENMP_MEM_FREE(PTR) omp_free(PTR, omp_null_allocator)
#else
# define ACC_OPENMP_MEM_ALLOC(SIZE) malloc(SIZE)
# define ACC_OPENMP_MEM_FREE(PTR) free(PTR)
#endif


#if defined(__cplusplus)
extern "C" {
#endif


int acc_host_mem_allocate(void** host_mem, size_t n, acc_stream_t* stream)
{
  /* TODO: currently not enqueued because another (function-)signature
   * which takes a host memory pointer is expecting a pointer rather
   * than a pointer to a pointer. The latter would be necessary to
   * allow for asynchronous delivery.
   */
  acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
  int result = (NULL != s ? s->status : EXIT_SUCCESS);
  assert(NULL != host_mem || 0 == n);
  if (EXIT_SUCCESS == result && 0 != n) {
    *host_mem = ACC_OPENMP_MEM_ALLOC(n);
    if (NULL == *host_mem) {
      if (NULL != s) {
#if defined(_OPENMP) && (200805 <= _OPENMP) /* OpenMP 3.0 */
#       pragma omp atomic write
#else
#       pragma omp critical
#endif
        s->status = s->status | EXIT_FAILURE;
      }
      result = EXIT_FAILURE;
    }
  }
  return result;
}


int acc_host_mem_deallocate(void* host_mem, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  if (NULL != host_mem) {
#if !defined(ACC_OPENMP_OFFLOAD)
    (void)(stream); /* unused */
#else /* implies _OPENMP */
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        assert(NULL != deps);
        deps->args[0].ptr = host_mem;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          int tid = 0;
          for (; tid < nthreads; ++tid) {
            acc_openmp_depend_t *const di = &deps[tid];
            const char *const id = di->in, *const od = di->out;
            (void)(id); (void)(od); /* suppress incorrect warning */
#           pragma omp task depend(in:id[0]) depend(out:od[0])
            ACC_OPENMP_MEM_FREE(di->args[0].ptr);
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    ACC_OPENMP_MEM_FREE(host_mem);
  }
  return result;
}


int acc_dev_mem_allocate(void** dev_mem, size_t n)
{
  assert(NULL != dev_mem);
#if defined(ACC_OPENMP_OFFLOAD)
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
#if defined(ACC_OPENMP_OFFLOAD)
    if (0 < omp_get_num_devices()) {
      omp_target_free(dev_mem, omp_get_default_device());
    }
    else
#endif
    {
      ACC_OPENMP_MEM_FREE(dev_mem);
    }
  }
  return EXIT_SUCCESS;
}


int acc_dev_mem_set_ptr(void** dev_mem, void* other, size_t lb)
{
  int result;
  assert(NULL != dev_mem);
  if (NULL != other || 0 == lb) {
    /* OpenMP specification: pointer arithmetic may not be valid */
    *dev_mem = (char*)other + lb;
    result = EXIT_SUCCESS;
  }
  else result = EXIT_FAILURE;
  return result;
}


int acc_memcpy_h2d(const void* host_mem, void* dev_mem, size_t count, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != host_mem && NULL != dev_mem) || 0 == count);
  if (0 != count) {
#if !defined(ACC_OPENMP_OFFLOAD)
    (void)(stream); /* unused */
#else /* implies _OPENMP */
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        deps->args[0].const_ptr = host_mem;
        deps->args[1].ptr = dev_mem;
        deps->args[2].size = count;
        deps->args[3].ptr = stream;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          /* capture current default device before spawning task (acc_set_active_device) */
          const int dev_src = omp_get_initial_device(), dev_dst = omp_get_default_device();
          int tid = 0;
          for (; tid < nthreads; ++tid) {
            acc_openmp_depend_t *const di = &deps[tid];
            const char *const id = di->in, *const od = di->out;
            acc_openmp_stream_t *const s = (acc_openmp_stream_t*)di->args[3].ptr; assert(NULL != s);
            (void)(id); (void)(od); /* suppress incorrect warning */
#           pragma omp task depend(in:id[0]) depend(out:od[0])
            s->status |= omp_target_memcpy(di->args[1].ptr, di->args[0]./*const_*/ptr, di->args[2].size,
              0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    memcpy(dev_mem, host_mem, count);
  }
  return result;
}


int acc_memcpy_d2h(const void* dev_mem, void* host_mem, size_t count, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_mem && NULL != host_mem) || 0 == count);
  if (0 != count) {
#if !defined(ACC_OPENMP_OFFLOAD)
    (void)(stream); /* unused */
#else /* implies _OPENMP */
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        deps->args[0].const_ptr = dev_mem;
        deps->args[1].ptr = host_mem;
        deps->args[2].size = count;
        deps->args[3].ptr = stream;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          /* capture current default device before spawning task (acc_set_active_device) */
          const int dev_src = omp_get_default_device(), dev_dst = omp_get_initial_device();
          int tid = 0;
          for (; tid < nthreads; ++tid) {
            acc_openmp_depend_t *const di = &deps[tid];
            const char *const id = di->in, *const od = di->out;
            acc_openmp_stream_t *const s = (acc_openmp_stream_t*)di->args[3].ptr; assert(NULL != s);
            (void)(id); (void)(od); /* suppress incorrect warning */
#           pragma omp task depend(in:id[0]) depend(out:od[0])
            s->status |= omp_target_memcpy(di->args[1].ptr, di->args[0]./*const_*/ptr, di->args[2].size,
              0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    memcpy(host_mem, dev_mem, count);
  }
  return result;
}


int acc_memcpy_d2d(const void* devmem_src, void* devmem_dst, size_t count, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != devmem_src && NULL != devmem_dst) || 0 == count);
  if (0 != count) {
#if !defined(ACC_OPENMP_OFFLOAD)
    (void)(stream); /* unused */
#else /* implies _OPENMP */
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        deps->args[0].const_ptr = devmem_src;
        deps->args[1].ptr = devmem_dst;
        deps->args[2].size = count;
        deps->args[3].ptr = stream;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          /* capture current default device before spawning task (acc_set_active_device) */
          const int dev_src = omp_get_default_device(), dev_dst = dev_src;
          int tid = 0;
          for (; tid < nthreads; ++tid) {
            acc_openmp_depend_t *const di = &deps[tid];
            const char *const id = di->in, *const od = di->out;
            acc_openmp_stream_t *const s = (acc_openmp_stream_t*)di->args[3].ptr; assert(NULL != s);
            (void)(id); (void)(od); /* suppress incorrect warning */
#           pragma omp task depend(in:id[0]) depend(out:od[0])
            s->status |= omp_target_memcpy(di->args[1].ptr, di->args[0]./*const_*/ptr, di->args[2].size,
              0/*dst_offset*/, 0/*src_offset*/, dev_dst, dev_src);
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    memcpy(devmem_dst, devmem_src, count);
  }
  return result;
}


int acc_memset_zero(void* dev_mem, size_t offset, size_t length, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert(NULL != dev_mem || 0 == length);
  if (0 != length) {
#if !defined(ACC_OPENMP_OFFLOAD)
    (void)(stream); /* unused */
#else /* implies _OPENMP */
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        deps->args[0].ptr = dev_mem;
        deps->args[1].size = offset;
        deps->args[2].size = length;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          int tid = 0;
          for (; tid < nthreads; ++tid) {
            acc_openmp_depend_t *const di = &deps[tid];
            /* OpenMP specification: pointer arithmetic may not be valid */
            char * /*const*/ dst = (char*)di->args[0].ptr + di->args[1].size;
            const size_t size = di->args[2].size; /* length */
            const char *const id = di->in, *const od = di->out;
            size_t i; /* private(i) */
#           pragma omp target teams distribute parallel for simd depend(in:id[0]) depend(out:od[0]) nowait is_device_ptr(dst)
            for (i = 0; i < size; ++i) dst[i] = '\0';
            (void)(id); (void)(od); /* suppress incorrect warning */
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    memset((char*)dev_mem + offset, 0, length);
  }
  return result;
}


int acc_dev_mem_info(size_t* mem_free, size_t* mem_total)
{
  int ndevices = 0, result = (NULL != mem_free || NULL != mem_total)
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
