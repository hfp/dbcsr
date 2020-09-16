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

#if defined(CL_VERSION_1_2)
# define ACC_OPENCL_ENQUEUE_EVENT(QUEUE, EVENT) clEnqueueMarkerWithWaitList(QUEUE, 0, NULL, EVENT)
#else
# define ACC_OPENCL_ENQUEUE_EVENT(QUEUE, EVENT) clEnqueueMarker(QUEUE, EVENT)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int acc_event_create(acc_event_t** event_p)
{
  cl_int result = EXIT_SUCCESS;
  const cl_event event = clCreateUserEvent(acc_opencl_context, &result);
  assert(NULL != event_p);
  if (NULL != event) {
    assert(CL_SUCCESS == result);
    *event_p = (acc_event_t*)malloc(sizeof(acc_event_t));
    if (NULL != *event_p) {
      (*event_p)->event = event;
      result = EXIT_SUCCESS;
    }
    else {
      clReleaseEvent(event);
      result = EXIT_FAILURE;
    }
  }
  else {
    assert(CL_SUCCESS != result);
    ACC_OPENCL_ERROR("failed to create user-defined event", result);
    *event_p = NULL;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_event_destroy(acc_event_t* event)
{
  int result = (NULL == event || NULL != event->event) ? EXIT_SUCCESS : EXIT_FAILURE;
  if (NULL != event) {
    ACC_OPENCL_CHECK(clReleaseEvent(event->event),
      "failed to release user-defined event", result);
    free(event);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_event_record(acc_event_t* event, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert(NULL != event && NULL != event->event);
  assert(NULL != stream && NULL != stream->queue);
  ACC_OPENCL_CHECK(ACC_OPENCL_ENQUEUE_EVENT(stream->queue, &event->event),
    "failed to record event", result);
  ACC_OPENCL_RETURN(result);
}


int acc_event_query(acc_event_t* event, acc_bool_t* has_occurred)
{
  int result = EXIT_SUCCESS;
  cl_int status = CL_QUEUED;
  assert(CL_COMPLETE != status);
  if (NULL != event) {
    ACC_OPENCL_CHECK(clGetEventInfo(event->event, CL_EVENT_COMMAND_EXECUTION_STATUS,
      sizeof(cl_int), &status, NULL), "failed to retrieve event status", result);
  }
  assert(NULL != has_occurred);
  if (EXIT_SUCCESS == result) {
    /* an unrecorded, i.e., an empty event has no work to wait for hence everything has occurred */
    *has_occurred = (CL_COMPLETE != status || CL_SUBMITTED != status ? 0 : 1);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_event_synchronize(acc_event_t* event)
{ /* Waits on the host-side. */
  int result = EXIT_SUCCESS;
  assert(NULL != event && NULL != event->event);
  ACC_OPENCL_CHECK(clWaitForEvents(1, &event->event),
    "failed to synchronize event", result);
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
