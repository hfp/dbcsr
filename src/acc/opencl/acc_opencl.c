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
#include <ctype.h>
#if defined(_OPENMP)
# include <omp.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

cl_device_id acc_opencl_devices[ACC_OPENCL_DEVICES_MAXCOUNT];
int acc_opencl_ndevices;
#if defined(_OPENMP)
# pragma omp threadprivate(acc_opencl_device)
#endif
int acc_opencl_device;


int acc_opencl_alloc(void** item, size_t typesize, int* counter, int maxcount, void* storage, void** pointer)
{
  int result, i;
  assert(NULL != item && 0 != typesize && NULL != counter);
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


int acc_opencl_dealloc(void* item, size_t typesize, int* counter, int maxcount, void* storage, void** pointer)
{
  int result;
  assert(0 != typesize && NULL != counter);
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


const char* acc_opencl_stristr(const char* a, const char* b)
{
  const char* result = NULL;
  if (NULL != a && NULL != b && '\0' != *a && '\0' != *b) {
    do {
      if (tolower(*a) != tolower(*b)) {
        ++a;
      }
      else {
        const char* c = b;
        result = a;
        while ('\0' != *++a && '\0' != *++c) {
          if (tolower(*a) != tolower(*c)) {
            result = NULL;
            break;
          }
        }
        if ('\0' == *c) break;
      }
    } while ('\0' != *a);
  }
  return result;
}


int acc_init(void)
{
  char buffer[ACC_OPENCL_STRING_MAXLENGTH];
  const char *const vendor = getenv("ACC_OPENCL_VENDOR");
  const char *const device = getenv("ACC_OPENCL_DEVICE");
  cl_platform_id platforms[ACC_OPENCL_PLATFORM_MAXCOUNT];
  cl_uint nplatforms = 0, ndevices = 0, i;
  cl_device_type type = CL_DEVICE_TYPE_ALL;
  ACC_OPENCL_CHECK(clGetPlatformIDs(0, NULL, &nplatforms), "failed to query number of platforms");
  ACC_OPENCL_CHECK(clGetPlatformIDs(
    nplatforms <= ACC_OPENCL_PLATFORM_MAXCOUNT ? nplatforms : ACC_OPENCL_PLATFORM_MAXCOUNT,
    platforms, 0), "failed to query platforms");
  if (NULL != device && '\0' != *device) {
    if (NULL != acc_opencl_stristr(device, "gpu")) {
      type = CL_DEVICE_TYPE_GPU;
    }
    else if (NULL != acc_opencl_stristr(device, "cpu")) {
      type = CL_DEVICE_TYPE_CPU;
    }
    else {
      type = CL_DEVICE_TYPE_ACCELERATOR;
    }
  }
  for (i = 0; i < nplatforms; ++i) {
    int n = 0;
    if (NULL != vendor && '\0' != *vendor) {
      size_t size;
      ACC_OPENCL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 0, NULL, &size), "failed to query platform vendor");
      buffer[0] = '\0'; size = (size <= ACC_OPENCL_STRING_MAXLENGTH ? size : ACC_OPENCL_STRING_MAXLENGTH);
      ACC_OPENCL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR,
        size, buffer, NULL), "failed to retrieve platform vendor");
      if (NULL == acc_opencl_stristr(buffer, vendor)) continue;
    }
    assert(acc_opencl_ndevices <= ACC_OPENCL_DEVICES_MAXCOUNT);
    ACC_OPENCL_CHECK(clGetDeviceIDs(platforms[i], type, 0, NULL, &ndevices), "failed to query number of devices");
    n = (acc_opencl_ndevices + ndevices) < ACC_OPENCL_DEVICES_MAXCOUNT ? (int)ndevices : (ACC_OPENCL_DEVICES_MAXCOUNT - acc_opencl_ndevices);
    ACC_OPENCL_CHECK(clGetDeviceIDs(platforms[i], type,
      n, acc_opencl_devices + acc_opencl_ndevices, NULL), "failed to query devices");
    acc_opencl_ndevices += n;
  }
  if (0 == acc_opencl_ndevices) acc_opencl_ndevices = -1;
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
#if 0 /* flush all pending work */
  ACC_OPENCL_EXPECT(EXIT_SUCCESS, acc_event_record(NULL/*event*/, NULL/*stream*/));
  acc_opencl_stream_clear_errors();
#endif
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
  int result;
  if (0 <= device_id && device_id < acc_opencl_ndevices) {
    acc_opencl_device = device_id;
    result = EXIT_SUCCESS;
  }
  else {
    result = EXIT_FAILURE;
  }
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
