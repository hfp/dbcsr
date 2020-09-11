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

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
acc_opencl_stream_t  acc_opencl_streams[ACC_OPENCL_STREAM_MAXCOUNT];
acc_opencl_stream_t* acc_opencl_streamp[ACC_OPENCL_STREAM_MAXCOUNT];
#endif
int acc_opencl_stream_count;


int acc_stream_create(acc_stream_t** stream_p, const char* name, int priority)
{
  acc_opencl_stream_t* stream;
  int result = acc_opencl_alloc((void**)&stream,
    sizeof(acc_opencl_stream_t), &acc_opencl_stream_count,
#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
    ACC_OPENCL_STREAM_MAXCOUNT, acc_opencl_streams, (void**)acc_opencl_streamp);
#else
    0, NULL, NULL);
#endif
#if defined(CL_QUEUE_PRIORITY_LOW_KHR) && defined(CL_QUEUE_PRIORITY_MED_KHR) && defined(CL_QUEUE_PRIORITY_HIGH_KHR)
  assert(CL_QUEUE_PRIORITY_HIGH_KHR <= priority && CL_QUEUE_PRIORITY_LOW_KHR >= priority);
#endif
  if (EXIT_SUCCESS == result) {
    assert(NULL != stream);
#if defined(ACC_OPENCL_STRING_MAXLENGTH) && (0 < ACC_OPENCL_STRING_MAXLENGTH) && !defined(NDEBUG)
    strncpy(stream->name, NULL != name ? name : "", ACC_OPENCL_STRING_MAXLENGTH);
    stream->name[ACC_OPENCL_STRING_MAXLENGTH-1] = '\0';
#else
    ACC_OPENCL_UNUSED(name);
#endif
    assert(NULL != stream_p);
    *stream_p = stream;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_stream_destroy(acc_stream_t* stream)
{
#if 0
  acc_opencl_stream_t *const s = (acc_opencl_stream_t*)stream;
  int result = ((NULL != s && 0 < s->pending) ? acc_stream_sync(stream) : EXIT_SUCCESS);
#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
  assert(NULL == s || (acc_opencl_streams <= s && s < (acc_opencl_streams + ACC_OPENCL_STREAM_MAXCOUNT)));
#endif
  if (EXIT_SUCCESS == result) {
    result = acc_opencl_dealloc(stream, sizeof(acc_opencl_stream_t), &acc_opencl_stream_count,
#if defined(ACC_OPENCL_STREAM_MAXCOUNT) && (0 < ACC_OPENCL_STREAM_MAXCOUNT)
      ACC_OPENCL_STREAM_MAXCOUNT, acc_opencl_streams, (void**)acc_opencl_streamp);
#else
      0, NULL, NULL);
#endif
  }
  ACC_OPENCL_RETURN(result);
#else
  return EXIT_FAILURE;
#endif
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
  int result;
  if (NULL != least || NULL != greatest) {
    assert(least != greatest); /* no alias */
    if (NULL != least) {
#if defined(CL_QUEUE_PRIORITY_LOW_KHR) && defined(CL_QUEUE_PRIORITY_MED_KHR) && defined(CL_QUEUE_PRIORITY_HIGH_KHR)
      *least = CL_QUEUE_PRIORITY_LOW_KHR;
#else
      *least = ACC_OPENCL_STREAM_PRIORITY_INVALID;
#endif
    }
    if (NULL != greatest) {
#if defined(CL_QUEUE_PRIORITY_LOW_KHR) && defined(CL_QUEUE_PRIORITY_MED_KHR) && defined(CL_QUEUE_PRIORITY_HIGH_KHR)
      *greatest = CL_QUEUE_PRIORITY_HIGH_KHR;
#else
      *greatest = ACC_OPENCL_STREAM_PRIORITY_INVALID;
#endif
    }
    result =
#if defined(CL_QUEUE_PRIORITY_LOW_KHR) && defined(CL_QUEUE_PRIORITY_MED_KHR) && defined(CL_QUEUE_PRIORITY_HIGH_KHR)
      (CL_QUEUE_PRIORITY_HIGH_KHR > CL_QUEUE_PRIORITY_MED_KHR || CL_QUEUE_PRIORITY_MED_KHR > CL_QUEUE_PRIORITY_LOW_KHR)
    ? EXIT_FAILURE :
#endif
      EXIT_SUCCESS;
  }
  else {
    result = EXIT_FAILURE;
  }
  ACC_OPENCL_RETURN(result);
}


int acc_stream_sync(acc_stream_t* stream)
{ /* Blocks the host-thread. */
#if 0
  int result = (NULL != stream ? EXIT_SUCCESS : EXIT_FAILURE);
  if (EXIT_SUCCESS == result) {
    result = acc_event_record(NULL/*event*/, stream);
    if (EXIT_SUCCESS == result) {
      acc_opencl_stream_t* const s = (acc_opencl_stream_t*)stream;
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
