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
#include <string.h>
#include <assert.h>
#include <ctype.h>
#if defined(_OPENMP)
# include <omp.h>
#endif
#if defined(_WIN32)
# include <windows.h>
# define ACC_OPENCL_PATHSEP "\\"
#else
# include <glob.h>
# define ACC_OPENCL_PATHSEP "/"
#endif


#if defined(__cplusplus)
extern "C" {
#endif

int acc_opencl_ndevices;
cl_device_id acc_opencl_devices[ACC_OPENCL_DEVICES_MAXCOUNT];
cl_context acc_opencl_context;


void acc_opencl_notify(const char* /*errinfo*/, const void* /*private_info*/, size_t /*cb*/, void* /*user_data*/);
void acc_opencl_notify(const char* errinfo, const void* private_info, size_t cb, void* user_data)
{
  ACC_OPENCL_UNUSED(private_info); ACC_OPENCL_UNUSED(cb); ACC_OPENCL_UNUSED(user_data);
  fprintf(stderr, "ERROR ACC/OpenCL: %s\n", errinfo);
}


/** Returns the pointer to the 1st match of "b" in "a". */
const char* acc_opencl_stristr(const char* /*a*/, const char* /*b*/);
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
  const char *const disable = getenv("ACC_OPENCL_DISABLE");
#if defined(_OPENMP) && defined(ACC_OPENCL_THREADLOCAL_CONTEXT)
  int result = (0 == omp_in_parallel() ? EXIT_SUCCESS : EXIT_FAILURE);
#else
  int result = EXIT_SUCCESS;
#endif
  if (NULL == disable || '0' == *disable) {
    cl_platform_id platforms[ACC_OPENCL_DEVICES_MAXCOUNT];
    char buffer[ACC_OPENCL_BUFFER_MAXSIZE];
    const char *const vendor = getenv("ACC_OPENCL_VENDOR");
    const char *const device = getenv("ACC_OPENCL_DEVICE");
    cl_uint nplatforms = 0, ndevices = 0, i;
    cl_device_type type = CL_DEVICE_TYPE_ALL;
    ACC_OPENCL_CHECK(clGetPlatformIDs(0, NULL, &nplatforms),
      "query number of platforms", result);
    ACC_OPENCL_CHECK(clGetPlatformIDs(
      nplatforms <= ACC_OPENCL_DEVICES_MAXCOUNT ? nplatforms : ACC_OPENCL_DEVICES_MAXCOUNT,
      platforms, 0), "retrieve platform ids", result);
    if (NULL != device && '\0' != *device) {
      if (NULL != acc_opencl_stristr(device, "gpu")) type = CL_DEVICE_TYPE_GPU;
      else if (NULL != acc_opencl_stristr(device, "cpu")) type = CL_DEVICE_TYPE_CPU;
      else type = CL_DEVICE_TYPE_ACCELERATOR;
    }
    acc_opencl_ndevices = 0;
    for (i = 0; i < nplatforms; ++i) {
      int n;
      if (NULL != vendor && '\0' != *vendor) {
        size_t size = 0;
        ACC_OPENCL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR,
          0, NULL, &size), "query platform vendor", result);
        buffer[0] = '\0'; size = (size <= ACC_OPENCL_BUFFER_MAXSIZE
          ? size : ACC_OPENCL_BUFFER_MAXSIZE);
        ACC_OPENCL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR,
          size, buffer, NULL), "retrieve platform vendor", result);
        if (NULL == acc_opencl_stristr(buffer, vendor)) continue;
      }
      assert(acc_opencl_ndevices <= ACC_OPENCL_DEVICES_MAXCOUNT);
      ACC_OPENCL_CHECK(clGetDeviceIDs(platforms[i], type, 0, NULL, &ndevices),
        "query number of devices", result);
      n = (acc_opencl_ndevices + ndevices) < ACC_OPENCL_DEVICES_MAXCOUNT
        ? (int)ndevices : (ACC_OPENCL_DEVICES_MAXCOUNT - acc_opencl_ndevices);
      ACC_OPENCL_CHECK(clGetDeviceIDs(platforms[i], type,
        n, acc_opencl_devices + acc_opencl_ndevices, NULL),
        "retrieve device ids", result);
      acc_opencl_ndevices += n;
    }
    assert(NULL == acc_opencl_context);
    if (0 < acc_opencl_ndevices) {
      int n = 0;
      if (1 < acc_opencl_ndevices) { /* preselect default device */
        for (n = 0; n < acc_opencl_ndevices; ++n) {
          ACC_OPENCL_CHECK(clGetDeviceInfo(acc_opencl_devices[n],
            CL_DEVICE_TYPE, sizeof(cl_device_type), &type, NULL),
            "retrieve device type", result);
          if (CL_DEVICE_TYPE_DEFAULT & type) break;
        }
      }
      if (EXIT_SUCCESS == result) {
        if (!(CL_DEVICE_TYPE_DEFAULT & type)) n = 0;
        result = acc_set_active_device(n);
#if defined(_OPENMP) && defined(ACC_OPENCL_THREADLOCAL_CONTEXT)
        if (EXIT_SUCCESS == result) {
          const cl_context context = acc_opencl_context;
#         pragma omp parallel
          if (context != acc_opencl_context) {
            if (CL_SUCCESS == clRetainContext(context)) {
              acc_opencl_context = context;
            }
            else {
              assert(CL_SUCCESS != result);
              ACC_OPENCL_ERROR("retain context", result);
              acc_opencl_context = NULL;
            }
          }
        }
#endif
      }
    }
    else { /* mark as initialized */
      acc_opencl_ndevices = -1;
    }
  }
  else { /* mark as initialized */
    acc_opencl_ndevices = -1;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_finalize(void)
{
#if defined(_OPENMP) && defined(ACC_OPENCL_THREADLOCAL_CONTEXT)
  int result = (0 == omp_in_parallel() ? EXIT_SUCCESS : EXIT_FAILURE);
#else
  int result = EXIT_SUCCESS;
#endif
  if (NULL != acc_opencl_context) {
    const cl_context context = acc_opencl_context;
    assert(0 < acc_opencl_ndevices);
#if defined(_OPENMP) && defined(ACC_OPENCL_THREADLOCAL_CONTEXT)
#   pragma omp parallel
    if (context != acc_opencl_context) {
      ACC_OPENCL_CHECK(clReleaseContext(acc_opencl_context),
        "release context", result);
      acc_opencl_context = NULL;
    }
#endif
    ACC_OPENCL_CHECK(clReleaseContext(context),
      "release context", result);
    acc_opencl_context = NULL;
  }
  ACC_OPENCL_RETURN(result);
}


void acc_clear_errors(void)
{
}


int acc_get_ndevices(int* ndevices)
{
  int result;
  if (NULL != ndevices && 0 != acc_opencl_ndevices) {
    *ndevices = (0 < acc_opencl_ndevices ? acc_opencl_ndevices : 0);
    result = EXIT_SUCCESS;
  }
  else {
    result = EXIT_FAILURE;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_opencl_device(cl_device_id* device)
{
  int result = EXIT_SUCCESS;
  assert(NULL != device);
  if (NULL != acc_opencl_context) {
#if !defined(NDEBUG)
    size_t n = 0;
#endif
    ACC_OPENCL_CHECK(clGetContextInfo(acc_opencl_context, CL_CONTEXT_DEVICES,
      sizeof(cl_device_id), device, &n), "retrieve id of active device", result);
    assert(EXIT_SUCCESS != result || sizeof(cl_device_id) == n/*single-device context*/);
  }
  else {
    *device = NULL;
  }
  return result;
}


int acc_set_active_device(int device_id)
{
  cl_int result = (((0 <= device_id && device_id < acc_opencl_ndevices) ||
    /* allow successful completion if no device was found */
    0 > acc_opencl_ndevices) ? EXIT_SUCCESS : EXIT_FAILURE);
  if (0 < acc_opencl_ndevices) {
    cl_device_id active_id = NULL;
    if (EXIT_SUCCESS == result) result = acc_opencl_device(&active_id);
    if (acc_opencl_devices[device_id] != active_id) {
      if (NULL != acc_opencl_context) {
        ACC_OPENCL_CHECK(clReleaseContext(acc_opencl_context),
          "release context", result);
      }
      if (EXIT_SUCCESS == result) {
        cl_context_properties properties[] = {
          /* insert other properties in front of below property */
          CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE, /* TODO */
          0 /* end of properties */
        };
        acc_opencl_context = clCreateContext(properties,
          1/*num_devices*/, acc_opencl_devices + device_id,
          acc_opencl_notify, NULL/* user_data*/,
          &result);
        if (CL_INVALID_VALUE == result) { /* retry */
          const size_t n = sizeof(properties) / sizeof(*properties);
          assert(3 <= n);
          properties[n-3] = 0;
          acc_opencl_context = clCreateContext(0 != properties[0] ? properties : NULL,
            1/*num_devices*/, acc_opencl_devices + device_id,
            acc_opencl_notify, NULL/* user_data*/,
            &result);
        }
        ACC_OPENCL_CHECK(result, "create context", result);
      }
    }
  }
  ACC_OPENCL_RETURN(result);
}


int acc_opencl_source_exists(const char* /*path*/, const char* /*fileext*/);
int acc_opencl_source_exists(const char* path, const char* fileext)
{
  int result;
  const char *const ext = (NULL != fileext ? fileext : "*." ACC_OPENCL_SRCEXT);
  if (NULL != path && '\0' != *path) {
    char filepath[ACC_OPENCL_BUFFER_MAXSIZE];
#if defined(_WIN32)
    const int nchar = ACC_OPENCL_SNPRINTF(filepath, ACC_OPENCL_BUFFER_MAXSIZE, "%s" ACC_OPENCL_PATHSEP "%s", path, ext);
    if (0 <= nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) {
      WIN32_FIND_DATA data;
      HANDLE handle = FindFirstFile(filepath, &data);
      if (INVALID_HANDLE_VALUE != handle) {
        result = EXIT_SUCCESS;
        FindClose(handle);
      }
      else {
        result = EXIT_FAILURE;
      }
    }
#else
    glob_t globbuf;
    const int nchar = ACC_OPENCL_SNPRINTF(filepath, ACC_OPENCL_BUFFER_MAXSIZE, "%s" ACC_OPENCL_PATHSEP "%s", path, ext);
    if (0 <= nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) {
      result = glob(filepath, 0/*flags*/, NULL, &globbuf);
      globfree(&globbuf);
    }
#endif
    else {
      result = EXIT_FAILURE;
    }
  }
  else {
    result = EXIT_FAILURE;
  }
  return result;
}


const char* acc_opencl_source_path(const char* fileext)
{
  const char *const ext = NULL != fileext ? fileext : ACC_OPENCL_SRCEXT;
  char pattern[ACC_OPENCL_BUFFER_MAXSIZE];
  const int nchar = ACC_OPENCL_SNPRINTF(pattern, ACC_OPENCL_BUFFER_MAXSIZE, "*.%s", ext);
  const char* result = NULL;
  if (0 <= nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) {
    if (EXIT_SUCCESS == acc_opencl_source_exists(getenv("ACC_OPENCL_SOURCE_PATH"), pattern)) {
      result = getenv("ACC_OPENCL_SOURCE_PATH");
    }
    else if (EXIT_SUCCESS == acc_opencl_source_exists(getenv("CP2K_DATA_DIR"), pattern)) {
      result = getenv("CP2K_DATA_DIR");
    }
  }
  return result;
}


FILE* acc_opencl_source_open(const char* filename, const char *const dirpaths[], int ndirpaths)
{
  char filepath[ACC_OPENCL_BUFFER_MAXSIZE];
  FILE* result = NULL;
  int i;
  assert(NULL != filename && (0 >= ndirpaths || NULL != dirpaths));
  for (i = 0; i < ndirpaths; ++i) {
    if (NULL != dirpaths[i]) {
      const int nchar = ACC_OPENCL_SNPRINTF(filepath, ACC_OPENCL_BUFFER_MAXSIZE, "%s" ACC_OPENCL_PATHSEP "%s", dirpaths[i], filename);
      result = ((0 <= nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) ? fopen(filepath, "r") : NULL);
      if (NULL != result) break;
    }
  }
  if (NULL == result) {
    const char *const dotext = strrchr(filename, '.');
    const char *const path = acc_opencl_source_path(NULL != dotext ? (dotext + 1) : NULL);
    if (NULL != path) {
      const int nchar = ACC_OPENCL_SNPRINTF(filepath, ACC_OPENCL_BUFFER_MAXSIZE, "%s" ACC_OPENCL_PATHSEP "%s", path, filename);
      result = ((0 <= nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) ? fopen(filepath, "r") : NULL);
    }
  }
  return result;
}


int acc_opencl_source(FILE* source, char* lines[], int max_nlines, int cleanup)
{
  int nlines = 0;
  if  ((NULL != lines && 0 < max_nlines)
    && (NULL != source || NULL != lines[0]))
  {
    char *input = (NULL != source ? ((char*)malloc(max_nlines * ACC_OPENCL_MAXLINELEN)) : lines[0]);
    int cleanup_begin = cleanup;
    while (nlines < max_nlines && NULL != input && (NULL == source
      || NULL != fgets(input, ACC_OPENCL_MAXLINELEN, source)))
    {
      char *const begin = input, *end = strchr(input, '\n');
      int inc = 1;
      lines[nlines] = input;
      if (NULL != source) {
        input += ACC_OPENCL_MAXLINELEN;
        if (NULL != end) *end = '\0';
      }
      else if (NULL != end) {
        input = end + 1;
        *end = '\0';
      }
      else input = NULL;
      if (0 != cleanup) {
        const char *const line = lines[nlines] + strspn(lines[nlines], " \t");
        size_t len = strlen(line);
        if (0 == len) inc = 0;
        else if (2 <= len) {
          if ('/' == line[0] && '/' == line[1]) inc = 0;
          else {
            end = strstr(line, "*/");
            if ('/' == line[0] && '*' == line[1]) {
              ++cleanup_begin;
              inc = 0;
            }
            if (NULL != end && '\0' == end[2+strspn(end + 2, " \t")]) {
              --cleanup_begin;
              inc = 0;
            }
          }
        }
        if (cleanup < cleanup_begin) inc = 0;
        if (0 == inc && 0 == nlines && NULL != source) input = begin;
      }
      nlines += inc;
    }
  }
  if (0 < max_nlines && NULL != lines) {
    lines[nlines] = NULL; /* terminator */
  }
  else if (0 == nlines && NULL != source) {
    free(lines[nlines]);
  }
  return nlines;
}


int acc_opencl_wgsize(cl_kernel kernel, size_t* preferred_multiple)
{
  cl_device_id active_id = NULL;
  int result = acc_opencl_device(&active_id);
  assert(NULL != preferred_multiple);
  ACC_OPENCL_CHECK(clGetKernelWorkGroupInfo(kernel, active_id,
    CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
    sizeof(size_t), preferred_multiple, NULL),
    "query preferred multiple of workgroup size", result);
  return result;
}


int acc_opencl_kernel(const char *const source[], int nlines, const char* build_options,
  const char* kernel_name, cl_kernel* kernel)
{
  cl_int result;
  assert(NULL != kernel);
  if (NULL != acc_opencl_context && 0 < nlines) {
    const cl_program program = clCreateProgramWithSource(
      acc_opencl_context, nlines, (const char**)source, NULL, &result);
    if (NULL != program) {
      cl_device_id active_id = NULL;
      assert(CL_SUCCESS == result);
      result = acc_opencl_device(&active_id);
      ACC_OPENCL_CHECK(clBuildProgram(program, 1/*num_devices*/, &active_id,
        build_options, NULL/*callback*/, NULL/*user_data*/),
        "build program", result);
      if (EXIT_SUCCESS == result) {
        *kernel = clCreateKernel(program, kernel_name, &result);
        ACC_OPENCL_ERROR("create kernel", result);
      }
    }
    else {
      assert(CL_SUCCESS != result);
      ACC_OPENCL_ERROR("create program", result);
      *kernel = NULL;
    }
  }
  else {
    result = EXIT_FAILURE;
    *kernel = NULL;
  }
  return result;
}

#if defined(__cplusplus)
}
#endif
