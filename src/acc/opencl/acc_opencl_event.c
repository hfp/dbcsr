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

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(ACC_OPENCL_EVENT_MAXCOUNT) && (0 < ACC_OPENCL_EVENT_MAXCOUNT)
acc_event_t  acc_opencl_events[ACC_OPENCL_EVENT_MAXCOUNT];
acc_event_t* acc_opencl_eventp[ACC_OPENCL_EVENT_MAXCOUNT];
#endif
int acc_opencl_event_count;


int acc_event_create(acc_event_t** event_p)
{
  cl_int result = EXIT_SUCCESS;
  const cl_event event = clCreateUserEvent(acc_opencl_context, &result);
  if (NULL != event) {
    assert(CL_SUCCESS == result);
    result = acc_opencl_alloc((void**)event_p,
      sizeof(acc_event_t), &acc_opencl_event_count,
#if defined(ACC_OPENCL_EVENT_MAXCOUNT) && (0 < ACC_OPENCL_EVENT_MAXCOUNT)
      ACC_OPENCL_EVENT_MAXCOUNT, acc_opencl_events, (void**)acc_opencl_eventp);
#else
      0, NULL, NULL);
#endif
    if (EXIT_SUCCESS == result) {
      assert(NULL != *event_p);
      (*event_p)->event = event;
    }
    else {
      clReleaseEvent(event);
    }
  }
  else {
    ACC_OPENCL_ERROR("failed to create user-defined event", result);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_event_destroy(acc_event_t* event)
{
#if 0
  return acc_opencl_dealloc(event, sizeof(acc_event_t), &acc_opencl_event_count,
#if defined(ACC_OPENCL_EVENT_MAXCOUNT) && (0 < ACC_OPENCL_EVENT_MAXCOUNT)
    ACC_OPENCL_EVENT_MAXCOUNT, acc_opencl_events, (void**)acc_opencl_eventp);
#else
    0, NULL, NULL);
#endif
#else
  return EXIT_FAILURE;
#endif
}


int acc_event_record(acc_event_t* event, acc_stream_t* stream)
{
#if 0
  int result;
  if (NULL != stream) {
    acc_event_t *const e = (acc_event_t*)event;
#if defined(ACC_OPENCL_OFFLOAD)
    if (0 < acc_opencl_ndevices()) {
      if (NULL != e) { /* reset if reused (re-enqueued) */
        e->dependency = deps->data.out;
      }
    }
    else
#endif
    if (NULL != e) {
      e->dependency = NULL;
      result = EXIT_SUCCESS;
    }
    else {
      stream->pending = 0;
      result = EXIT_SUCCESS;
    }
  }
  else if (NULL == event) { /* flush all pending work */
    result = EXIT_SUCCESS;
#if defined(ACC_OPENCL_OFFLOAD) && 0
#   pragma omp master
#   pragma omp task if(0)
    result = EXIT_FAILURE;
#elif defined(_OPENMP)
#   pragma omp barrier
#endif
  }
  else result = EXIT_FAILURE;
  ACC_OPENCL_RETURN(result);
#else
  return EXIT_FAILURE;
#endif
}


int acc_event_query(acc_event_t* event, acc_bool_t* has_occurred)
{
#if 0
  int result = EXIT_FAILURE;
  if (NULL != has_occurred) {
    if (NULL != event) {
      const acc_event_t *const e = (acc_event_t*)event;
      *has_occurred = (NULL == e->dependency);
      result = EXIT_SUCCESS;
    }
    else *has_occurred = 0;
  }
  ACC_OPENCL_RETURN(result);
#else
  return EXIT_FAILURE;
#endif
}


int acc_event_synchronize(acc_event_t* event)
{ /* Waits on the host-side. */
#if 0
  const acc_event_t *const e = (acc_event_t*)event;
  int result;
#if defined(ACC_OPENCL_OFFLOAD)
  if (0 < acc_opencl_ndevices()) {
    if (NULL != e) {
    }
  }
  else
#endif
  if (NULL != e) {
    result = EXIT_SUCCESS;
  }
  else result = EXIT_FAILURE;
  ACC_OPENCL_RETURN(result);
#else
  return EXIT_FAILURE;
#endif
}

#if defined(__cplusplus)
}
#endif
