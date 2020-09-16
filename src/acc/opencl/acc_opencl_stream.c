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
# define ACC_OPENCL_CREATE_COMMAND_QUEUE(CTX, DEV, PROPS, RESULT) \
    clCreateCommandQueueWithProperties(CTX, DEV, (const cl_queue_properties*)(PROPS), RESULT)
#else
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

int acc_stream_create(acc_stream_t** stream_p, const char* name, int priority)
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
        cl_int properties[] = {
          CL_QUEUE_PRIORITY_KHR, 0/*placeholder filled-in below*/,
          0
        };
        properties[1] = priority;
        queue = ACC_OPENCL_CREATE_COMMAND_QUEUE(acc_opencl_context, device_id, properties, &result);
      }
      else
#endif
      {
        /*const cl_int properties[] = { 0 };*/
        queue = ACC_OPENCL_CREATE_COMMAND_QUEUE(acc_opencl_context, device_id, NULL/*properties*/, &result);
      }
    }
    assert(NULL != stream_p);
    if (NULL != queue) {
      assert(CL_SUCCESS == result);
      *stream_p = (acc_stream_t*)malloc(sizeof(acc_stream_t));
      if (NULL != *stream_p) {
#if defined(ACC_OPENCL_STRING_MAXLENGTH) && (0 < ACC_OPENCL_STRING_MAXLENGTH) && !defined(NDEBUG)
        strncpy((*stream_p)->name, NULL != name ? name : "", ACC_OPENCL_STRING_MAXLENGTH);
        (*stream_p)->name[ACC_OPENCL_STRING_MAXLENGTH-1] = '\0';
#else
        ACC_OPENCL_UNUSED(name);
#endif
        (*stream_p)->queue = queue;
        result = EXIT_SUCCESS;
      }
      else {
        clReleaseCommandQueue(queue);
        result = EXIT_FAILURE;
      }
    }
    else {
      assert(CL_SUCCESS != result);
      ACC_OPENCL_ERROR("failed to create command queue", result);
      *stream_p = NULL;
    }
  }
  ACC_OPENCL_RETURN(result);
}


int acc_stream_destroy(acc_stream_t* stream)
{
  int result = (NULL == stream || NULL != stream->queue) ? EXIT_SUCCESS : EXIT_FAILURE;
  if (NULL != stream) {
    ACC_OPENCL_CHECK(clReleaseCommandQueue(stream->queue),
      "failed to release command queue", result);
    free(stream);
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


int acc_stream_sync(acc_stream_t* stream)
{ /* Blocks the host-thread. */
  int result = EXIT_SUCCESS;
  ACC_OPENCL_CHECK(clFinish(NULL != stream ? stream->queue : NULL),
    "failed to synchronize stream", result);
  ACC_OPENCL_RETURN(result);
}


int acc_stream_wait_event(acc_stream_t* stream, acc_event_t* event)
{ /* Wait for an event (device-side). */
  int result = EXIT_SUCCESS;
  assert(NULL != stream && NULL != stream->queue);
  assert(NULL != event && NULL != event->event);
  ACC_OPENCL_CHECK(ACC_OPENCL_WAIT_EVENT(stream->queue, &event->event),
    "failed to wait for an event", result);
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
