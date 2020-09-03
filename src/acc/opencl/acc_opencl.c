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
#if defined(_OPENMP)
# include <omp.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int acc_opencl_ndevices;
int acc_opencl_device;


int acc_opencl_alloc(void** item, int typesize, int* counter, int maxcount, void* storage, void** pointer)
{
  int result, i;
  assert(NULL != item && 0 < typesize && NULL != counter);
#if defined(_OPENMP) && (200805 <= _OPENMP) /* OpenMP 3.0 */
# pragma omp atomic capture
#elif defined(_OPENMP)
# pragma omp critical(acc_opencl_alloc_critical)
#endif
  i = (*counter)++;
  if (0 < maxcount) { /* fast allocation */
    if (i < maxcount) {
      assert(NULL != storage && NULL != pointer);
      *item = pointer[i];
      if (NULL == *item) {
        *item = &((char*)storage)[i*typesize];
      }
      assert(((const char*)storage) <= ((const char*)*item) && ((const char*)*item) < &((const char*)storage)[maxcount*typesize]);
      result = EXIT_SUCCESS;
    }
    else { /* out of space */
      result = EXIT_FAILURE;
#if defined(_OPENMP)
#     pragma omp atomic
#endif
      --(*counter);
      *item = NULL;
    }
  }
  else {
    *item = malloc(typesize);
    if (NULL != *item) {
      result = EXIT_SUCCESS;
    }
    else {
      result = EXIT_FAILURE;
#if defined(_OPENMP)
#     pragma omp atomic
#endif
      --(*counter);
    }
  }
  ACC_OPENCL_RETURN(result);
}


int acc_opencl_dealloc(void* item, int typesize, int* counter, int maxcount, void* storage, void** pointer)
{
  int result;
  assert(0 < typesize && NULL != counter);
  if (NULL != item) {
    int i;
#if defined(_OPENMP) && (200805 <= _OPENMP) /* OpenMP 3.0 */
#   pragma omp atomic capture
#elif defined(_OPENMP)
#   pragma omp critical(acc_opencl_alloc_critical)
#endif
    i = (*counter)--;
    assert(0 <= i);
    if (0 < maxcount) { /* fast allocation */
      assert(NULL != storage && NULL != pointer);
      result = ((0 <= i && i < maxcount && storage <= item && /* check if item came from storage */
          ((const char*)item) < &((const char*)storage)[maxcount*typesize])
        ? EXIT_SUCCESS : EXIT_FAILURE);
      pointer[i] = item;
    }
    else {
      result = EXIT_SUCCESS;
      free(item);
    }
  }
  else {
    result = EXIT_SUCCESS;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_init(void)
{
  //acc_opencl_device = getenv("ACC_OPENCL_DEVICE");
  cl_uint nplatforms = 1;
  ACC_OPENCL_CHECK(clGetPlatformIDs(0, 0, &nplatforms), "failed to query number of platforms");
#if 0
  std::vector<cl_platform_id> platforms(nplatforms);
  ACC_OPENCL_CHECK(clGetPlatformIDs(nplatforms, &platforms[0], 0), "failed to query platform IDs");
  const char *const vendor = (opencl_vendor && *opencl_vendor) ? opencl_vendor : getenv("OCL_VENDOR");
  cl_uint platform_id = 0;
  if (1 != nplatforms && vendor && *vendor) {
    std::vector<char> v(vendor, vendor + std::char_traits<char>::length(vendor) + 1);
    tolower(&v[0]);
    std::vector<char> info;
    while (platform_id < nplatforms) {
      std::size_t size;
      ACC_OPENCL_CHECK(clGetPlatformInfo(platforms[platform_id], CL_PLATFORM_VENDOR, 0, 0, &size), "failed to query platform vendor");
      if (info.size() * sizeof(char) < size) {
        info.resize(std::max(info.size(), size / sizeof(char)));
      }
      ACC_OPENCL_CHECK(clGetPlatformInfo(platforms[platform_id], CL_PLATFORM_VENDOR, size, &info[0], 0), "failed to retrieve platform vendor");
      tolower(&info[0], &info[0] + size);
      if (0 != strstr(&info[0], &v[0])) {
        break;
      }
      ++platform_id;
    }
  }

  local::OclPlatform = platforms[platform_id];
#endif

#if defined(_OPENMP)
  assert(/*master*/0 == omp_get_thread_num());
#endif
  ACC_OPENCL_RETURN((0 != acc_opencl_ndevices
    && 0 == acc_opencl_stream_count
    && 0 == acc_opencl_event_count)
  ? EXIT_SUCCESS : EXIT_FAILURE);
}


int acc_finalize(void)
{
#if defined(_OPENMP)
  assert(/*master*/0 == omp_get_thread_num());
#endif
  ACC_OPENCL_RETURN((0 != acc_opencl_ndevices
    && 0 == acc_opencl_stream_count
    && 0 == acc_opencl_event_count)
  ? EXIT_SUCCESS : EXIT_FAILURE);
}


void acc_clear_errors(void)
{ assert(0 != acc_opencl_ndevices);
  /* flush all pending work */
  //ACC_OPENCL_EXPECT(EXIT_SUCCESS, acc_event_record(NULL/*event*/, NULL/*stream*/));
  //acc_opencl_stream_clear_errors();
}


int acc_get_ndevices(int* n_devices)
{
  int result;
  if (NULL != n_devices && 0 != acc_opencl_ndevices) {
    *n_devices = (0 < acc_opencl_ndevices ? acc_opencl_ndevices : 0);
    result = EXIT_SUCCESS;
  }
  else {
    result = EXIT_FAILURE;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_set_active_device(int device_id)
{
#if 0
  const int device = ((NULL == acc_opencl_device || 0 == *acc_opencl_device)
    ? device_id : (device_id + atoi(acc_opencl_device)));
  int result = (0 <= device ? EXIT_SUCCESS : EXIT_FAILURE);
#if defined(_OPENMP)
# pragma omp master
#endif
  if (EXIT_SUCCESS == result && 0 != acc_opencl_ndevices) {
#if !defined(NDEBUG)
    if (device_id < acc_opencl_ndevices)
#endif
    {
      //omp_set_default_device(device);
      result = EXIT_SUCCESS;
    }
#if !defined(NDEBUG)
    else {
      result = EXIT_FAILURE;
    }
#endif
  }
  ACC_OPENCL_RETURN(result);
#else
  return -1;
#endif
}

#if defined(__cplusplus)
}
#endif
