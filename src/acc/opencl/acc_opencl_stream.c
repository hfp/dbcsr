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

#if !defined(ACC_OPENCL_STREAM_PRIORITY_INVALID)
# define ACC_OPENCL_STREAM_PRIORITY_INVALID -1
#endif

#if defined(CL_VERSION_2_0)
# define ACC_OPENCL_CREATE_COMMAND_QUEUE clCreateCommandQueueWithProperties
#else
# define ACC_OPENCL_CREATE_COMMAND_QUEUE clCreateCommandQueue
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
acc_stream_t  acc_opencl_streams[ACC_OPENCL_STREAM_MAXCOUNT];
acc_stream_t* acc_opencl_streamp[ACC_OPENCL_STREAM_MAXCOUNT];
#endif
int acc_opencl_stream_count;


int acc_stream_create(acc_stream_t** stream_p, const char* name, int priority)
{
  cl_int result = (NULL != acc_opencl_context ? EXIT_SUCCESS : EXIT_FAILURE);
  cl_command_queue_properties properties[] = {
#if defined(CL_QUEUE_PRIORITY_KHR)
    ACC_OPENCL_STREAM_PRIORITY_INVALID != priority ? CL_QUEUE_PRIORITY_KHR : 0,
    ACC_OPENCL_STREAM_PRIORITY_INVALID != priority ? priority : 0,
#endif
    0
  };
  cl_command_queue queue = NULL;
  cl_device_id device_id = NULL;
  size_t n = 0;
  assert(ACC_OPENCL_STREAM_PRIORITY_INVALID == priority ||
    (CL_QUEUE_PRIORITY_HIGH_KHR <= priority && CL_QUEUE_PRIORITY_LOW_KHR >= priority));
  ACC_OPENCL_CHECK(clGetContextInfo(acc_opencl_context, CL_CONTEXT_DEVICES,
    sizeof(cl_device_id), &device_id, &n), "failed to retrieve id of active device", result);
  assert(EXIT_SUCCESS != result || sizeof(cl_device_id) == n/*single-device context*/);
  if (EXIT_SUCCESS == result) {
    queue = ACC_OPENCL_CREATE_COMMAND_QUEUE(acc_opencl_context,
      device_id, properties, &result);
  }
  else {
    ACC_OPENCL_ERROR("failed to create OpenCL command queue", result);
  }
  if (NULL != queue) {
    assert(NULL != stream_p);
    result = acc_opencl_alloc((void**)stream_p, sizeof(acc_stream_t), &acc_opencl_stream_count,
#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
      ACC_OPENCL_STREAM_MAXCOUNT, acc_opencl_streams, (void**)acc_opencl_streamp);
#else
      0, NULL, NULL);
#endif
    if (EXIT_SUCCESS == result) {
      assert(NULL != *stream_p);
#if defined(ACC_OPENCL_STRING_MAXLENGTH) && (0 < ACC_OPENCL_STRING_MAXLENGTH) && !defined(NDEBUG)
      strncpy((*stream_p)->name, NULL != name ? name : "", ACC_OPENCL_STRING_MAXLENGTH);
      (*stream_p)->name[ACC_OPENCL_STRING_MAXLENGTH-1] = '\0';
#else
      ACC_OPENCL_UNUSED(name);
#endif
      (*stream_p)->queue = queue;
    }
  }
  ACC_OPENCL_RETURN(result);
}


int acc_stream_destroy(acc_stream_t* stream)
{
  int result;
#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
  assert(NULL == stream || (acc_opencl_streams <= stream && stream < (acc_opencl_streams + ACC_OPENCL_STREAM_MAXCOUNT)));
#endif
  result = acc_opencl_dealloc(stream, sizeof(acc_stream_t), &acc_opencl_stream_count,
#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
    ACC_OPENCL_STREAM_MAXCOUNT, acc_opencl_streams, (void**)acc_opencl_streamp);
#else
    0, NULL, NULL);
#endif
  ACC_OPENCL_RETURN(result);
}


void acc_opencl_stream_clear_errors(void)
{
#if 0
#if defined(_OPENMP)
# pragma omp critical
#endif
  {
#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
    int i = 0;
    for (; i < ACC_OPENCL_STREAM_MAXCOUNT; ++i) {
      acc_opencl_streams[i].status = EXIT_SUCCESS;
    }
#endif
  }
#endif
}


int acc_stream_priority_range(int* least, int* greatest)
{
  int result = ((NULL != least || NULL != greatest) ? EXIT_SUCCESS : EXIT_FAILURE);
#if defined(CL_QUEUE_PRIORITY_KHR)
  char buffer[ACC_OPENCL_BUFFER_MAXSIZE];
  cl_context_properties *const properties = (cl_context_properties*)buffer;
  cl_platform_id platform = NULL;
  size_t size = 0;
  if (CL_SUCCESS == clGetContextInfo(acc_opencl_context, CL_CONTEXT_PROPERTIES,
    ACC_OPENCL_BUFFER_MAXSIZE, properties, &size))
  {
    size_t i;
    for (i = 0; i < size; ++i) if (CL_CONTEXT_PLATFORM == properties[i]) {
      assert((i + 1) < size && 0 != properties[i+1]);
      platform = (cl_platform_id)properties[i+1];
    }
  }
  else {
    ACC_OPENCL_ERROR("failed to retrieve context properties", result);
  }
  if (NULL != platform) {
    if (CL_SUCCESS == clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS,
      ACC_OPENCL_BUFFER_MAXSIZE, buffer, NULL))
    {
      if (NULL != strstr(buffer, "cl_khr_priority_hints")) {
        if (NULL != least) *least = CL_QUEUE_PRIORITY_LOW_KHR;
        if (NULL != greatest) *greatest = CL_QUEUE_PRIORITY_HIGH_KHR;
      }
      else
#endif
      {
        if (NULL != least) *least = ACC_OPENCL_STREAM_PRIORITY_INVALID;
        if (NULL != greatest) *greatest = ACC_OPENCL_STREAM_PRIORITY_INVALID;
      }
#if defined(CL_QUEUE_PRIORITY_KHR)
    }
    else {
      ACC_OPENCL_ERROR("failed to retrieve platform extensions", result);
    }
  }
#endif
  assert(least != greatest); /* no alias */
  ACC_OPENCL_RETURN(result);
}


int acc_stream_sync(acc_stream_t* stream)
{ /* Blocks the host-thread. */
#if 0
  int result = (NULL != stream ? EXIT_SUCCESS : EXIT_FAILURE);
  if (EXIT_SUCCESS == result) {
    result = acc_event_record(NULL/*event*/, stream);
    if (EXIT_SUCCESS == result) {
    }
  }
  ACC_OPENCL_RETURN(result);
#else
  return EXIT_FAILURE;
#endif
}


int acc_stream_wait_event(acc_stream_t* stream, acc_event_t* event)
{ /* Waits (device-side) for an event. */
#if 0
  int result;
  if (NULL != stream && NULL != event) {
#if defined(ACC_OPENCL_OFFLOAD)
    if (0 < acc_opencl_ndevices()) {
    }
    else
#endif
    result = acc_event_synchronize(event);
  }
  else result = (NULL == event ? EXIT_SUCCESS : EXIT_FAILURE);
  ACC_OPENCL_RETURN(result);
#else
  return EXIT_FAILURE;
#endif
}

#if defined(__cplusplus)
}
#endif
