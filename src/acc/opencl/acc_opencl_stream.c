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
/* can depend on OpenCL implementation */
#if !defined(ACC_OPENCL_STREAM_NOALLOC) && 1
# define ACC_OPENCL_STREAM_NOALLOC
#endif

#if defined(CL_VERSION_2_0)
# define ACC_OPENCL_COMMAND_QUEUE_PROPERTIES cl_queue_properties
# define ACC_OPENCL_CREATE_COMMAND_QUEUE(CTX, DEV, PROPS, RESULT) \
    clCreateCommandQueueWithProperties(CTX, DEV, PROPS, RESULT)
#else
# define ACC_OPENCL_COMMAND_QUEUE_PROPERTIES cl_int
# define ACC_OPENCL_CREATE_COMMAND_QUEUE(CTX, DEV, PROPS, RESULT) \
    clCreateCommandQueue(CTX, DEV, /* avoid warning about unused argument */ \
      (cl_command_queue_properties)(NULL != (PROPS) ? (((cl_int*)(PROPS))[sizeof(PROPS)/sizeof(cl_int)-1]) : 0), RESULT)
#endif

#if defined(CL_VERSION_1_2)
# define ACC_OPENCL_WAIT_EVENT(QUEUE, EVENT) clEnqueueMarkerWithWaitList(QUEUE, 1, EVENT, NULL)
#else
# define ACC_OPENCL_WAIT_EVENT(QUEUE, EVENT) clEnqueueWaitForEvents(QUEUE, 1, EVENT)
#endif


#if defined(__cplusplus)
extern "C" {
#endif

int acc_stream_create(void** stream_p, const char* name, int priority)
{
  cl_int result = EXIT_SUCCESS;
  if (NULL != acc_opencl_context) {
    cl_command_queue queue = NULL;
    cl_device_id device_id = NULL;
    size_t n = 0;
#if defined(CL_QUEUE_PRIORITY_KHR)
    assert(ACC_OPENCL_STREAM_PRIORITY_INVALID == priority ||
      (CL_QUEUE_PRIORITY_HIGH_KHR <= priority && CL_QUEUE_PRIORITY_LOW_KHR >= priority));
#else
    assert(ACC_OPENCL_STREAM_PRIORITY_INVALID == priority);
#endif
    ACC_OPENCL_CHECK(clGetContextInfo(acc_opencl_context, CL_CONTEXT_DEVICES,
      sizeof(cl_device_id), &device_id, &n), "failed to retrieve id of active device", result);
    assert(EXIT_SUCCESS != result || sizeof(cl_device_id) == n/*single-device context*/);
    if (EXIT_SUCCESS == result) {
#if defined(CL_QUEUE_PRIORITY_KHR)
      if (ACC_OPENCL_STREAM_PRIORITY_INVALID != priority) {
        ACC_OPENCL_COMMAND_QUEUE_PROPERTIES properties[] = {
          CL_QUEUE_PRIORITY_KHR, 0/*placeholder filled-in below*/,
          0
        };
        properties[1] = priority;
        queue = ACC_OPENCL_CREATE_COMMAND_QUEUE(acc_opencl_context, device_id, properties, &result);
      }
      else
#endif
      {
        /*ACC_OPENCL_COMMAND_QUEUE_PROPERTIES properties[] = { 0 };*/
        queue = ACC_OPENCL_CREATE_COMMAND_QUEUE(acc_opencl_context, device_id, NULL/*properties*/, &result);
      }
    }
    assert(NULL != stream_p);
    if (NULL != queue) {
      assert(CL_SUCCESS == result);
#if defined(ACC_OPENCL_STREAM_NOALLOC)
      assert(sizeof(void*) >= sizeof(cl_command_queue));
      *stream_p = (void*)queue;
#else
      *stream_p = malloc(sizeof(cl_command_queue));
      if (NULL != *stream_p) {
        ACC_OPENCL_UNUSED(name);
        *stream_p = (void*)queue;
        result = EXIT_SUCCESS;
      }
      else {
        clReleaseCommandQueue(queue);
        result = EXIT_FAILURE;
      }
#endif
    }
    else {
      assert(CL_SUCCESS != result);
      ACC_OPENCL_ERROR("failed to create command queue", result);
      *stream_p = NULL;
    }
  }
  ACC_OPENCL_RETURN(result);
}


int acc_stream_destroy(void* stream)
{
  int result = EXIT_SUCCESS;
  if (NULL != stream) {
    ACC_OPENCL_CHECK(clReleaseCommandQueue((cl_command_queue)stream),
      "failed to release command queue", result);
#if defined(ACC_OPENCL_STREAM_NOALLOC)
    assert(sizeof(void*) >= sizeof(cl_command_queue));
#else
    free(stream);
#endif
  }
  ACC_OPENCL_RETURN(result);
}


int acc_stream_priority_range(int* least, int* greatest)
{
  int result = ((NULL != least || NULL != greatest) ? EXIT_SUCCESS : EXIT_FAILURE);
  if (NULL != acc_opencl_context) {
#if defined(CL_QUEUE_PRIORITY_KHR)
    char buffer[ACC_OPENCL_BUFFER_MAXSIZE];
    cl_platform_id platform = NULL;
    cl_device_id active_id = NULL;
    size_t n = 0;
    assert(0 < acc_opencl_ndevices);
    ACC_OPENCL_CHECK(clGetContextInfo(acc_opencl_context, CL_CONTEXT_DEVICES,
      sizeof(cl_device_id), &active_id, &n), "failed to retrieve id of active device", result);
    ACC_OPENCL_CHECK(clGetDeviceInfo(active_id, CL_DEVICE_PLATFORM,
      sizeof(cl_platform_id), &platform, NULL), "failed to retrieve device platform", result);
    ACC_OPENCL_CHECK(clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS,
      ACC_OPENCL_BUFFER_MAXSIZE, buffer, NULL), "failed to retrieve platform extensions", result);
    if (EXIT_SUCCESS == result) {
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
#endif
  }
  else {
    if (NULL != least) *least = ACC_OPENCL_STREAM_PRIORITY_INVALID;
    if (NULL != greatest) *greatest = ACC_OPENCL_STREAM_PRIORITY_INVALID;
  }
  assert(least != greatest); /* no alias */
  ACC_OPENCL_RETURN(result);
}


int acc_stream_sync(void* stream)
{ /* Blocks the host-thread. */
  int result = EXIT_SUCCESS;
  assert(NULL != stream);
  ACC_OPENCL_CHECK(clFinish((cl_command_queue)stream),
    "failed to synchronize stream", result);
  ACC_OPENCL_RETURN(result);
}


int acc_stream_wait_event(void* stream, void* event)
{ /* Wait for an event (device-side). */
  int result = EXIT_SUCCESS;
  assert(NULL != stream && NULL != event);
  ACC_OPENCL_CHECK(ACC_OPENCL_WAIT_EVENT((cl_command_queue)stream, (cl_event*)&event),
    "failed to wait for an event", result);
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
