/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_opencl.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#if defined(_WIN32)
# include <Windows.h>
#else
# if !defined(__linux__)
#   include <sys/types.h>
#   include <sys/sysctl.h>
# endif
# include <unistd.h>
#endif

#define ACC_OPENCL_MEM_ALLOC(SIZE) malloc(SIZE)
#define ACC_OPENCL_MEM_FREE(PTR) free(PTR)

#if !defined(ACC_OPENCL_MEM_ALIGNSCALE)
# define ACC_OPENCL_MEM_ALIGNSCALE 8
#endif
#if !defined(ACC_OPENCL_DEVMEMSET) && 1
# define ACC_OPENCL_DEVMEMSET
#endif
#if !defined(ACC_OPENCL_MAP_MULTI) && 0
# define ACC_OPENCL_MAP_MULTI
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct acc_opencl_meminfo_t {
  cl_mem buffer;
  void* mapped;
} acc_opencl_meminfo_t;


int acc_opencl_memalignment(size_t /*size*/);
int acc_opencl_memalignment(size_t size)
{
  int result;
  if ((ACC_OPENCL_MEM_ALIGNSCALE * ACC_OPENCL_MAXALIGN_NBYTES) <= size) {
    result = ACC_OPENCL_MAXALIGN_NBYTES;
  }
  else if ((ACC_OPENCL_MEM_ALIGNSCALE * ACC_OPENCL_CACHELINE_NBYTES) <= size) {
    result = ACC_OPENCL_CACHELINE_NBYTES;
  }
  else {
    result = sizeof(void*);
  }
  return result;
}


acc_opencl_meminfo_t* acc_opencl_meminfo(void* /*memory*/);
acc_opencl_meminfo_t* acc_opencl_meminfo(void* memory)
{
  assert(NULL != memory && sizeof(acc_opencl_meminfo_t) <= (uintptr_t)memory);
  return (acc_opencl_meminfo_t*)((char*)memory - sizeof(acc_opencl_meminfo_t));
}


int acc_host_mem_allocate(void** host_mem, size_t nbytes, acc_stream_t* stream)
{
  cl_int result;
  const int alignment = acc_opencl_memalignment(nbytes);
  const size_t size = nbytes + alignment + sizeof(acc_opencl_meminfo_t) - 1;
  const cl_mem buffer = clCreateBuffer(acc_opencl_context, CL_MEM_ALLOC_HOST_PTR, size,
    NULL/*host_ptr*/, &result);
  assert(NULL != host_mem && NULL != stream);
  if (NULL != buffer) {
    const uintptr_t address = (uintptr_t)clEnqueueMapBuffer(stream->queue, buffer,
      CL_FALSE/*non-blocking*/, CL_MAP_READ | CL_MAP_WRITE,
      0/*offset*/, size, 0, NULL, NULL, &result);
    if (0 != address) {
      const size_t offset = ACC_OPENCL_UP2(address + sizeof(acc_opencl_meminfo_t), alignment) - address;
      acc_opencl_meminfo_t* meminfo;
      assert(sizeof(acc_opencl_meminfo_t) <= offset);
#if defined(ACC_OPENCL_MAP_MULTI)
      meminfo = (acc_opencl_meminfo_t*)clEnqueueMapBuffer(stream->queue, buffer,
        CL_TRUE/*blocking*/, CL_MAP_READ | CL_MAP_WRITE,
        offset - sizeof(acc_opencl_meminfo_t),
        sizeof(acc_opencl_meminfo_t),
        0, NULL, NULL, &result);
#else
      meminfo = (acc_opencl_meminfo_t*)(address + offset - sizeof(acc_opencl_meminfo_t));
#endif
      if (NULL != meminfo) {
        meminfo->buffer = buffer;
        meminfo->mapped = (void*)address;
        *host_mem = (void*)(address + offset);
      }
      else {
        assert(CL_SUCCESS != result);
        ACC_OPENCL_ERROR("failed to map buffer info", result);
        *host_mem = NULL;
      }
    }
    else {
      assert(CL_SUCCESS != result);
      ACC_OPENCL_ERROR("failed to map host buffer", result);
      *host_mem = NULL;
    }
  }
  else {
    assert(CL_SUCCESS != result);
    ACC_OPENCL_ERROR("failed to create host buffer", result);
    *host_mem = NULL;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_host_mem_deallocate(void* host_mem, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert(NULL != stream);
  if (NULL != host_mem) {
    acc_opencl_meminfo_t *const meminfo = acc_opencl_meminfo(host_mem);
    const acc_opencl_meminfo_t info = *meminfo; /* copy meminfo prior to unmap */
#if defined(ACC_OPENCL_MAP_MULTI)
    ACC_OPENCL_CHECK(clEnqueueUnmapMemObject(stream->queue, meminfo->buffer, meminfo,
      0, NULL, NULL), "failed to unmap memory info", result);
#endif
    ACC_OPENCL_CHECK(clEnqueueUnmapMemObject(stream->queue, info.buffer, info.mapped,
      0, NULL, NULL), "failed to unmap host memory", result);
    ACC_OPENCL_CHECK(clReleaseMemObject(info.buffer),
      "failed to release host memory buffer", result);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_dev_mem_allocate(void** dev_mem, size_t nbytes)
{
  cl_int result;
  const cl_mem buffer = clCreateBuffer(acc_opencl_context, CL_MEM_READ_WRITE, nbytes,
    NULL/*host_ptr*/, &result);
  assert(sizeof(void*) >= sizeof(cl_mem)); /* can depend on OpenCL implementation */
  assert(NULL != dev_mem);
  if (NULL != buffer) {
    *dev_mem = (void*)buffer;
  }
  else {
    assert(CL_SUCCESS != result);
    ACC_OPENCL_ERROR("failed to create device buffer", result);
    *dev_mem = NULL;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_dev_mem_deallocate(void* dev_mem)
{
  int result = EXIT_SUCCESS;
  assert(sizeof(void*) >= sizeof(cl_mem)); /* can depend on OpenCL implementation */
  if (NULL != dev_mem) {
    ACC_OPENCL_CHECK(clReleaseMemObject((cl_mem)dev_mem),
      "failed to release device memory buffer", result);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_dev_mem_set_ptr(void** dev_mem, void* other, size_t lb)
{
  int result;
  assert(NULL != dev_mem);
  if (NULL != other || 0 == lb) {
    *dev_mem = (char*)other + lb;
    result = EXIT_SUCCESS;
  }
  else result = EXIT_FAILURE;
  ACC_OPENCL_RETURN(result);
}


int acc_memcpy_h2d(const void* host_mem, void* dev_mem, size_t count, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != host_mem || 0 == count) && (NULL != dev_mem || 0 == count) && NULL != stream);
  assert(sizeof(void*) >= sizeof(cl_mem)); /* can depend on OpenCL implementation */
  if (NULL != host_mem && NULL != dev_mem && 0 != count) {
    ACC_OPENCL_CHECK(clEnqueueWriteBuffer(stream->queue, (cl_mem)dev_mem, CL_FALSE/*non-blocking*/,
      0/*offset*/, count, host_mem, 0, NULL, NULL), "failed to enqueue h2d copy", result);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_memcpy_d2h(const void* dev_mem, void* host_mem, size_t count, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_mem || 0 == count) && (NULL != host_mem || 0 == count) && NULL != stream);
  assert(sizeof(void*) >= sizeof(cl_mem)); /* can depend on OpenCL implementation */
  if (NULL != host_mem && NULL != dev_mem && 0 != count) {
    ACC_OPENCL_CHECK(clEnqueueReadBuffer(stream->queue, (cl_mem)dev_mem, CL_FALSE/*non-blocking*/,
      0/*offset*/, count, host_mem, 0, NULL, NULL), "failed to enqueue d2h copy", result);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_memcpy_d2d(const void* devmem_src, void* devmem_dst, size_t count, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != devmem_src || 0 == count) && (NULL != devmem_dst || 0 == count) && NULL != stream);
  assert(sizeof(void*) >= sizeof(cl_mem)); /* can depend on OpenCL implementation */
  if (NULL != devmem_src && NULL != devmem_dst && 0 != count) {
    ACC_OPENCL_CHECK(clEnqueueCopyBuffer(stream->queue, (cl_mem)devmem_src, (cl_mem)devmem_dst,
      0/*src_offset*/, 0/*dst_offset*/, count, 0, NULL, NULL),
      "failed to enqueue d2d copy", result);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_memset_zero(void* dev_mem, size_t offset, size_t length, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_mem || 0 == length) && NULL != stream);
  assert(sizeof(void*) >= sizeof(cl_mem)); /* can depend on OpenCL implementation */
  if (NULL != dev_mem) {
    const cl_uchar pattern = 0; /* fill with zeros */
    ACC_OPENCL_CHECK(clEnqueueFillBuffer(stream->queue, (cl_mem)dev_mem,
      &pattern, sizeof(pattern), offset, length, 0, NULL, NULL),
      "failed to enqueue zero-filling buffer", result);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_dev_mem_info(size_t* mem_free, size_t* mem_total)
{
  int ndevices = 0, result = (NULL != mem_free || NULL != mem_total)
    ? acc_get_ndevices(&ndevices) : EXIT_FAILURE;
  if (EXIT_SUCCESS == result) {
    size_t size_free = 0, size_total = 0;





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
# if defined(__linux__)
#   if defined(_SC_AVPHYS_PAGES)
    const long pages_free = sysconf(_SC_AVPHYS_PAGES);
#   else
    const long pages_free = 0;
#   endif
#   if defined(_SC_PHYS_PAGES)
    const long pages_total = sysconf(_SC_PHYS_PAGES);
#   else
    const long pages_total = pages_free;
#   endif
# else
    /*const*/ size_t size_pages_free = sizeof(const long), size_pages_total = sizeof(const long);
    long pages_free = 0, pages_total = 0;
    ACC_OPENCL_EXPECT(0, sysctlbyname("vm.page_free_count", &pages_free, &size_pages_free, NULL, 0));
    if (0 < page_size && 0 == sysctlbyname("hw.memsize", &pages_total, &size_pages_total, NULL, 0)) {
      pages_total /= page_size;
    }
    else pages_total = pages_free;
# endif
    if (0 < page_size && 0 <= pages_free && 0 <= pages_total) {
      const size_t size_page = (size_t)page_size;
      size_total = size_page * (size_t)pages_total;
      size_free  = size_page * (size_t)pages_free;
    }
    else result = EXIT_FAILURE;
#endif
    if (0 < ndevices) {
      size_total /= ndevices;
      size_free  /= ndevices;
    }
    if (size_free <= size_total) { /* EXIT_SUCCESS != result is ok */
      if (NULL != mem_total) *mem_total = size_total;
      if (NULL != mem_free)  *mem_free  = size_free;
    }
    else if (EXIT_SUCCESS == result) {
      result = EXIT_FAILURE;
    }
  }
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
