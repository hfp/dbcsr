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
      "failed to query number of platforms", result);
    ACC_OPENCL_CHECK(clGetPlatformIDs(
      nplatforms <= ACC_OPENCL_DEVICES_MAXCOUNT ? nplatforms : ACC_OPENCL_DEVICES_MAXCOUNT,
      platforms, 0), "failed to retrieve platforms", result);
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
          0, NULL, &size), "failed to query platform vendor", result);
        buffer[0] = '\0'; size = (size <= ACC_OPENCL_BUFFER_MAXSIZE
          ? size : ACC_OPENCL_BUFFER_MAXSIZE);
        ACC_OPENCL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR,
          size, buffer, NULL), "failed to retrieve platform vendor", result);
        if (NULL == acc_opencl_stristr(buffer, vendor)) continue;
      }
      assert(acc_opencl_ndevices <= ACC_OPENCL_DEVICES_MAXCOUNT);
      ACC_OPENCL_CHECK(clGetDeviceIDs(platforms[i], type, 0, NULL, &ndevices),
        "failed to query number of devices", result);
      n = (acc_opencl_ndevices + ndevices) < ACC_OPENCL_DEVICES_MAXCOUNT
        ? (int)ndevices : (ACC_OPENCL_DEVICES_MAXCOUNT - acc_opencl_ndevices);
      ACC_OPENCL_CHECK(clGetDeviceIDs(platforms[i], type,
        n, acc_opencl_devices + acc_opencl_ndevices, NULL),
        "failed to retrieve devices", result);
      acc_opencl_ndevices += n;
    }
    assert(NULL == acc_opencl_context);
    if (0 < acc_opencl_ndevices) {
      int n = 0;
      if (1 < acc_opencl_ndevices) { /* preselect default device */
        for (n = 0; n < acc_opencl_ndevices; ++n) {
          ACC_OPENCL_CHECK(clGetDeviceInfo(acc_opencl_devices[n],
            CL_DEVICE_TYPE, sizeof(cl_device_type), &type, NULL),
            "failed to retrieve device type", result);
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
              ACC_OPENCL_ERROR("failed to retain context", result);
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
        "failed to release context", result);
      acc_opencl_context = NULL;
    }
#endif
    ACC_OPENCL_CHECK(clReleaseContext(context),
      "failed to release context", result);
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


int acc_set_active_device(int device_id)
{
  cl_int result = (((0 <= device_id && device_id < acc_opencl_ndevices) ||
    /* allow successful completion if no device was found */
    0 > acc_opencl_ndevices) ? EXIT_SUCCESS : EXIT_FAILURE);
  if (0 < acc_opencl_ndevices) {
    cl_device_id active_id = NULL;
    size_t n = 0;
    if (NULL != acc_opencl_context) {
      ACC_OPENCL_CHECK(clGetContextInfo(acc_opencl_context, CL_CONTEXT_DEVICES,
        sizeof(cl_device_id), &active_id, &n), "failed to retrieve id of active device", result);
      assert(EXIT_SUCCESS != result || sizeof(cl_device_id) == n/*single-device context*/);
    }
    if (acc_opencl_devices[device_id] != active_id) {
      if (NULL != acc_opencl_context) {
        ACC_OPENCL_CHECK(clReleaseContext(acc_opencl_context),
          "failed to release context", result);
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
          n = sizeof(properties) / sizeof(*properties);
          assert(3 <= n);
          properties[n-3] = 0;
          acc_opencl_context = clCreateContext(0 != properties[0] ? properties : NULL,
            1/*num_devices*/, acc_opencl_devices + device_id,
            acc_opencl_notify, NULL/* user_data*/,
            &result);
        }
        ACC_OPENCL_CHECK(result, "failed to create context", result);
      }
    }
  }
  ACC_OPENCL_RETURN(result);
}


int acc_opencl_template(FILE* source, char* lines[], int max_nlines, int skip_lines,
  const char* type, const int params[], int nparams)
{
  int nlines = 0;
  if  (((NULL != source || (0 < max_nlines && NULL != lines[0])))
    && ((NULL != lines && 0 < max_nlines) || 0 >= max_nlines)
    && ((NULL != params && 0 < nparams) || 0 >= nparams)
    && ((NULL != type && 0 != *type) || NULL == type)
    && ((0 <= skip_lines)))
  {
    if (0 < max_nlines) {
      char *const buffer = (char*)malloc(max_nlines * ACC_OPENCL_MAXLINELEN);
      char format[ACC_OPENCL_MAXLINELEN], *line = lines[0];
      const int* param = params;
      lines[0] = buffer;
      while (nlines < max_nlines && NULL != lines[nlines]
        && (NULL == source || NULL != fgets(lines[nlines], ACC_OPENCL_MAXLINELEN, source)))
      {
        if (NULL == source) {
          char* end = strchr(line, '\n');
          if (NULL == end) end = strchr(line, '\0');
          if (NULL != end) {
            const int size = end - line;
            memcpy(lines[nlines], line, size);
            lines[nlines][size+0] = '\n';
            lines[nlines][size+1] = '\0';
            line += size + 1;
          }
        }
        if (0 == skip_lines) {
          char* subst = ((NULL != type || 0 < nparams) ? strchr(lines[nlines], '%') : NULL);
          size_t size = strlen(lines[nlines]) + 1;
          int next = nlines + 1, offset;
          while (NULL != subst) {
            const int len = subst - lines[nlines] + 2;
            const char c = lines[nlines][len];
            if (NULL != type && 's' == subst[1]) {
              memcpy(format, lines[nlines], size);
              format[len] = '\0';
              offset = ACC_OPENCL_SNPRINTF(lines[nlines], ACC_OPENCL_MAXLINELEN, format, type);
              type = NULL;
            }
            else if (0 < nparams && 'i' == subst[1]) {
              memcpy(format, lines[nlines], size);
              format[len] = '\0';
              offset = ACC_OPENCL_SNPRINTF(lines[nlines], ACC_OPENCL_MAXLINELEN, format, *param);
              --nparams;
              ++param;
            }
            else { /* unexpected placeholder */
              subst = NULL;
              offset = 0;
            }
            if (0 < offset && offset < ACC_OPENCL_MAXLINELEN) { /* try next */
              format[len] = c;
              memcpy(lines[nlines] + offset, format + len, size - len);
              subst = ((NULL != type || 0 < nparams) ? strchr(lines[nlines] + offset, '%') : NULL);
              size = offset + size - len;
            }
            else if (0 != offset) { /* error */
              if (NULL != source) free(lines[0]);
              lines[0] = NULL;
              subst = NULL;
              next = 0;
            }
          }
          if (next != nlines) { /* no error */
            assert(NULL != lines[0]);
            if (1 < size || '\0' != *lines[nlines]) {
              if (next < max_nlines) lines[next] = lines[nlines] + size;
              nlines = next;
            }
            else break;
          }
        }
        else { /* skip line */
          --skip_lines;
        }
      }
    }
  }
  if (0 < max_nlines && NULL != lines) {
    lines[nlines] = NULL; /* terminator */
  }
  return nlines;
}

#if defined(__cplusplus)
}
#endif
