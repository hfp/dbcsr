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

int acc_opencl_ndevices;
cl_platform_id acc_opencl_platforms[ACC_OPENCL_DEVICES_MAXCOUNT];
cl_device_id acc_opencl_devices[ACC_OPENCL_DEVICES_MAXCOUNT];
cl_context acc_opencl_context;


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
#if defined(ACC_OPENCL_STRING_MAXLENGTH) && (0 < ACC_OPENCL_STRING_MAXLENGTH)
  char buffer[ACC_OPENCL_STRING_MAXLENGTH];
  const char *const vendor = getenv("ACC_OPENCL_VENDOR");
#endif
  const char *const device = getenv("ACC_OPENCL_DEVICE");
  cl_uint nplatforms = 0, ndevices = 0, i;
  cl_device_type type = CL_DEVICE_TYPE_ALL;
  int n, result = (0 == acc_opencl_stream_count
                && 0 == acc_opencl_event_count
#if defined(_OPENMP)
                && /*master*/0 == omp_get_thread_num()
#endif
  ) ? EXIT_SUCCESS : EXIT_FAILURE;
  ACC_OPENCL_CHECK(clGetPlatformIDs(0, NULL, &nplatforms),
    "failed to query number of platforms", result);
  ACC_OPENCL_CHECK(clGetPlatformIDs(
    nplatforms <= ACC_OPENCL_DEVICES_MAXCOUNT ? nplatforms : ACC_OPENCL_DEVICES_MAXCOUNT,
    acc_opencl_platforms, 0), "failed to retrieve platforms", result);
  if (NULL != device && '\0' != *device) {
    if (NULL != acc_opencl_stristr(device, "gpu")) type = CL_DEVICE_TYPE_GPU;
    else if (NULL != acc_opencl_stristr(device, "cpu")) type = CL_DEVICE_TYPE_CPU;
    else type = CL_DEVICE_TYPE_ACCELERATOR;
  }
  acc_opencl_ndevices = 0;
  for (i = 0; i < nplatforms; ++i) {
#if defined(ACC_OPENCL_STRING_MAXLENGTH) && (0 < ACC_OPENCL_STRING_MAXLENGTH)
    if (NULL != vendor && '\0' != *vendor) {
      size_t size = 0;
      ACC_OPENCL_CHECK(clGetPlatformInfo(acc_opencl_platforms[i], CL_PLATFORM_VENDOR,
        0, NULL, &size), "failed to query platform vendor", result);
      buffer[0] = '\0'; size = (size <= ACC_OPENCL_STRING_MAXLENGTH
        ? size : ACC_OPENCL_STRING_MAXLENGTH);
      ACC_OPENCL_CHECK(clGetPlatformInfo(acc_opencl_platforms[i], CL_PLATFORM_VENDOR,
        size, buffer, NULL), "failed to retrieve platform vendor", result);
      if (NULL == acc_opencl_stristr(buffer, vendor)) continue;
    }
#endif
    assert(acc_opencl_ndevices <= ACC_OPENCL_DEVICES_MAXCOUNT);
    ACC_OPENCL_CHECK(clGetDeviceIDs(acc_opencl_platforms[i], type, 0, NULL, &ndevices),
      "failed to query number of devices", result);
    n = (acc_opencl_ndevices + ndevices) < ACC_OPENCL_DEVICES_MAXCOUNT
      ? (int)ndevices : (ACC_OPENCL_DEVICES_MAXCOUNT - acc_opencl_ndevices);
    ACC_OPENCL_CHECK(clGetDeviceIDs(acc_opencl_platforms[i], type,
      n, acc_opencl_devices + acc_opencl_ndevices, NULL),
      "failed to retrieve devices", result);
    acc_opencl_ndevices += n;
  }
  assert(NULL == acc_opencl_context);
  if (0 < acc_opencl_ndevices) {
    n = 0;
    if (1 < acc_opencl_ndevices) { /* preselect default device */
      for (n = 0; n < acc_opencl_ndevices; ++n) {
        ACC_OPENCL_CHECK(clGetDeviceInfo(acc_opencl_devices[n],
          CL_DEVICE_TYPE, sizeof(cl_device_type), &type, NULL),
          "failed to retrieve device information", result);
        if (CL_DEVICE_TYPE_DEFAULT & type) break;
      }
    }
    if (!(CL_DEVICE_TYPE_DEFAULT & type)) n = 0;
    if (EXIT_SUCCESS != acc_set_active_device(n)) {
      acc_opencl_ndevices = 0; /* raise error below */
    }
  }
  else { /* mark as initialized */
    acc_opencl_ndevices = -1;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_finalize(void)
{
  int result = (0 == acc_opencl_stream_count
             && 0 == acc_opencl_event_count
#if defined(_OPENMP)
             && /*master*/0 == omp_get_thread_num()
#endif
  ) ? EXIT_SUCCESS : EXIT_FAILURE;
  if (NULL != acc_opencl_context) {
    ACC_OPENCL_CHECK(clReleaseContext(acc_opencl_context),
      "failed to release OpenCL context", result);
  }
  ACC_OPENCL_RETURN(result);
}


void acc_clear_errors(void)
{
  assert(0 != acc_opencl_ndevices);
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
  cl_int result = ((0 <= device_id && device_id < acc_opencl_ndevices) ? EXIT_SUCCESS : EXIT_FAILURE);
  cl_device_id current_id = NULL;
  size_t n = 0;
  if (NULL != acc_opencl_context) {
    ACC_OPENCL_CHECK(clGetContextInfo(acc_opencl_context, CL_CONTEXT_DEVICES,
      sizeof(cl_device_id), &current_id, &n), "failed to retrieve id of active device", result);
    assert(EXIT_SUCCESS != result || sizeof(cl_device_id) == n/*single-device context*/);
  }
  if (acc_opencl_devices[device_id] != current_id) {
    cl_context_properties properties[] = {
      CL_CONTEXT_PLATFORM, (cl_context_properties)acc_opencl_platforms[device_id],
      CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE, /* TODO */
      0 /* end of properties */
    };
    if (NULL != acc_opencl_context) {
      ACC_OPENCL_CHECK(clReleaseContext(acc_opencl_context),
        "failed to release OpenCL context", result);
    }
    if (EXIT_SUCCESS == result) {
      acc_opencl_context = clCreateContext(properties,
        1/*num_devices*/, acc_opencl_devices + device_id,
        NULL/*pfn_notify*/, NULL/* user_data*/,
        &result);
      if (CL_INVALID_VALUE == result) { /* retry */
        n = sizeof(properties) / sizeof(*properties);
        assert(3 <= n);
        properties[n-3] = 0;
        acc_opencl_context = clCreateContext(properties,
          1/*num_devices*/, acc_opencl_devices + device_id,
          NULL/*pfn_notify*/, NULL/* user_data*/,
          &result);
      }
      ACC_OPENCL_CHECK(result, "failed to create OpenCL context", result);
    }
  }
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
