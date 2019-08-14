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

#if defined(__cplusplus)
extern "C" {
#endif

acc_openmp_stream_t  acc_openmp_streams[ACC_OPENMP_STREAM_MAXCOUNT];
acc_openmp_stream_t* acc_openmp_streamp[ACC_OPENMP_STREAM_MAXCOUNT];
int acc_openmp_stream_count;


int acc_stream_create(acc_stream_t* stream_p, const char* name, int priority)
{
  int result;
#if !defined(ACC_OPENMP)
  (void)(stream_p); (void)(name); (void)(priority); /* unused */
#else
  acc_openmp_stream_t *const stream = (acc_openmp_stream_t*)acc_openmp_alloc(
    sizeof(acc_openmp_stream_t), acc_openmp_streams, (void**)acc_openmp_streamp,
    &acc_openmp_stream_count, ACC_OPENMP_STREAM_MAXCOUNT);
  if (NULL != stream) {
    strncpy(stream->name, name, ACC_OPENMP_STRING_MAXLEN);
    stream->name[ACC_OPENMP_STRING_MAXLEN-1] = 0;
    stream->priority = priority;
    *stream_p = stream;
    result = EXIT_SUCCESS;
  }
  else
#endif
  {
    result = EXIT_FAILURE;
  }
  return result;
}


int acc_stream_destroy(acc_stream_t stream)
{
  int result;
#if defined(ACC_OPENMP)
  result = acc_openmp_dealloc(stream,
    sizeof(acc_openmp_stream_t), acc_openmp_streams, (void**)acc_openmp_streamp,
    &acc_openmp_stream_count, ACC_OPENMP_STREAM_MAXCOUNT);
#else
  (void)(stream); /* unused */
  result = EXIT_FAILURE;
#endif  
  return result;
}


int acc_stream_priority_range(int* least, int* greatest)
{
  return EXIT_FAILURE;
}


int acc_stream_sync(acc_stream_t stream)
{
  return EXIT_FAILURE;
}


int acc_stream_wait_event(acc_stream_t stream, acc_event_t event)
{
  return EXIT_FAILURE;
}

#if defined(__cplusplus)
}
#endif
