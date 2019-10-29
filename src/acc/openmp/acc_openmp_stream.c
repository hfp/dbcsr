/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_openmp.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

acc_openmp_stream_t  acc_openmp_streams[ACC_OPENMP_STREAM_MAXCOUNT];
acc_openmp_stream_t* acc_openmp_streamp[ACC_OPENMP_STREAM_MAXCOUNT];
int acc_openmp_stream_count;


int acc_openmp_stream_depend(acc_stream_t stream, acc_openmp_depend_t* in, acc_openmp_depend_t* out)
{
  int result;
  acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
  assert(NULL == s || (acc_openmp_streams <= s && s < acc_openmp_streams + ACC_OPENMP_STREAM_MAXCOUNT));
  if (NULL != stream && (NULL != in || NULL != out)) {
    if (NULL != out) {
      int index;
#if defined(ACC_OPENMP)
#     pragma omp atomic capture
#endif
      index = s->pending++;
      *out = s->name + index % ACC_OPENMP_STREAM_MAXPENDING;
    }
    if (NULL != in) {
      static const char dummy = 0;
      *in = ((NULL != out && s->name < *out) ? (*out - 1) : &dummy);
    }
#if defined(ACC_OPENMP) && !defined(NDEBUG)
    if (omp_get_default_device() != s->device_id) {
      result = EXIT_FAILURE;
    }
    else
#endif
    result = EXIT_SUCCESS;
  }
  else {
    result = EXIT_FAILURE;
  }
  return result;
}


int acc_stream_create(acc_stream_t* stream_p, const char* name, int priority)
{
  int result;
  acc_openmp_stream_t *const stream = (acc_openmp_stream_t*)acc_openmp_alloc(
    sizeof(acc_openmp_stream_t), acc_openmp_streams, (void**)acc_openmp_streamp,
    &acc_openmp_stream_count, ACC_OPENMP_STREAM_MAXCOUNT);
  if (NULL != stream) {
    strncpy(stream->name, name, ACC_OPENMP_STREAM_MAXPENDING);
    stream->name[ACC_OPENMP_STREAM_MAXPENDING-1] = 0;
    stream->priority = priority;
    stream->pending = 0;
    stream->status = 0;
#if defined(ACC_OPENMP) && !defined(NDEBUG)
    stream->device_id = omp_get_default_device();
#endif
    *stream_p = stream;
    result = EXIT_SUCCESS;
  }
  else {
    result = EXIT_FAILURE;
  }
  return result;
}


int acc_stream_destroy(acc_stream_t stream)
{
  int result;
  acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
  assert(NULL == s || (acc_openmp_streams <= s && s < acc_openmp_streams + ACC_OPENMP_STREAM_MAXCOUNT));
  result = ((NULL != stream && 0 < s->pending) ? acc_stream_sync(stream) : EXIT_SUCCESS);
  if (EXIT_SUCCESS == result) {
    result = acc_openmp_dealloc(stream,
      sizeof(acc_openmp_stream_t), acc_openmp_streams, (void**)acc_openmp_streamp,
      &acc_openmp_stream_count, ACC_OPENMP_STREAM_MAXCOUNT);
  }
  return result;
}


int acc_stream_priority_range(int* least, int* greatest)
{
  int result;
  if (NULL != least || NULL != greatest) {
    if (NULL != least) {
      *least = -1;
    }
    if (NULL != greatest) {
      *greatest = -1;
    }
    result = EXIT_SUCCESS;
  }
  else {
    result = EXIT_FAILURE;
  }
  return result;
}


int acc_stream_sync(acc_stream_t stream)
{
  int result;
  acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
  assert(NULL == s || (acc_openmp_streams <= s && s < acc_openmp_streams + ACC_OPENMP_STREAM_MAXCOUNT));
  if (NULL != stream && 0 < s->pending) {
    result = EXIT_FAILURE; /* TODO */
  }
  else {
    result = EXIT_SUCCESS;
  }
  return result;
}


int acc_stream_wait_event(acc_stream_t stream, acc_event_t event)
{
  int result;
  acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
  assert(NULL == s || (acc_openmp_streams <= s && s < acc_openmp_streams + ACC_OPENMP_STREAM_MAXCOUNT));
  result = EXIT_FAILURE; /* TODO */
  return result;
}

#if defined(__cplusplus)
}
#endif
