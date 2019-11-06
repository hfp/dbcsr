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

acc_openmp_depend_t acc_openmp_stream_call[ACC_OPENMP_THREADS_MAXCOUNT];
#if defined(ACC_OPENMP_STREAM_MAXCOUNT) && (0 < ACC_OPENMP_STREAM_MAXCOUNT)
acc_openmp_stream_t  acc_openmp_streams[ACC_OPENMP_STREAM_MAXCOUNT];
acc_openmp_stream_t* acc_openmp_streamp[ACC_OPENMP_STREAM_MAXCOUNT];
#endif
int acc_openmp_stream_count;


int acc_openmp_stream_depend(acc_stream_t stream, acc_openmp_depend_t** depend)
{
  int result;
  acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
#if defined(ACC_OPENMP_STREAM_MAXCOUNT) && (0 < ACC_OPENMP_STREAM_MAXCOUNT)
  assert(NULL == s || (acc_openmp_streams <= s && s < (acc_openmp_streams + ACC_OPENMP_STREAM_MAXCOUNT)));
#endif
#if defined(ACC_OPENMP_OFFLOAD) && !defined(NDEBUG)
  assert(omp_get_default_device() == s->device_id);
#endif
  assert(NULL != depend);
  if (NULL != s && EXIT_SUCCESS == s->status) {
    static const char dummy = 0;
    int index;
#if !defined(_OPENMP)
    acc_openmp_depend_t *const d = acc_openmp_stream_call;
#else
    acc_openmp_depend_t *const d = &acc_openmp_stream_call[omp_get_thread_num()];
# if defined(_OPENMP) && (200805 <= _OPENMP) /* OpenMP 3.0 */
#   pragma omp atomic capture
# else
#   pragma omp critical(acc_openmp_stream_depend_critical)
# endif
#endif
    index = s->pending++;
    assert(NULL != d);
#if !defined(NDEBUG)
    memset(d->args, 0, ACC_OPENMP_ARGUMENTS_MAXCOUNT * sizeof(acc_openmp_any_t));
#endif
    d->out = s->name + index % ACC_OPENMP_STREAM_MAXPENDING;
    d->in = (s->name < d->out ? (d->out - 1) : &dummy);
    *depend = d;
    result = EXIT_SUCCESS;
  }
  else {
#if !defined(NDEBUG) /* user checks return value */
    *depend = NULL;
#endif
    result = (NULL != s ? s->status : EXIT_SUCCESS);
  }
  return result;
}


int acc_stream_create(acc_stream_t* stream_p, const char* name, int priority)
{
  acc_openmp_stream_t* stream;
  const int result = acc_openmp_alloc((void**)&stream,
    sizeof(acc_openmp_stream_t), &acc_openmp_stream_count,
#if defined(ACC_OPENMP_STREAM_MAXCOUNT) && (0 < ACC_OPENMP_STREAM_MAXCOUNT)
    ACC_OPENMP_STREAM_MAXCOUNT, acc_openmp_streams, (void**)acc_openmp_streamp);
#else
    0, NULL, NULL);
#endif
  assert(NULL != stream_p);
  if (EXIT_SUCCESS == result) {
    assert(NULL != stream);
    strncpy(stream->name, name, ACC_OPENMP_STREAM_MAXPENDING);
    stream->name[ACC_OPENMP_STREAM_MAXPENDING-1] = '\0';
    stream->priority = priority;
    stream->pending = 0;
    stream->status = 0;
#if defined(ACC_OPENMP_OFFLOAD) && !defined(NDEBUG)
    stream->device_id = omp_get_default_device();
#endif
    *stream_p = stream;
  }
  return result;
}


int acc_stream_destroy(acc_stream_t stream)
{
  acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
  int result = ((NULL != s && 0 < s->pending) ? acc_stream_sync(stream) : EXIT_SUCCESS);
#if defined(ACC_OPENMP_STREAM_MAXCOUNT) && (0 < ACC_OPENMP_STREAM_MAXCOUNT)
  assert(NULL == s || (acc_openmp_streams <= s && s < (acc_openmp_streams + ACC_OPENMP_STREAM_MAXCOUNT)));
#endif
  if (EXIT_SUCCESS == result) {
    result = acc_openmp_dealloc(stream, sizeof(acc_openmp_stream_t), &acc_openmp_stream_count,
#if defined(ACC_OPENMP_STREAM_MAXCOUNT) && (0 < ACC_OPENMP_STREAM_MAXCOUNT)
      ACC_OPENMP_STREAM_MAXCOUNT, acc_openmp_streams, (void**)acc_openmp_streamp);
#else
      0, NULL, NULL);
#endif
  }
  return result;
}


int acc_openmp_stream_clear_errors(void)
{
#if defined(ACC_OPENMP_STREAM_MAXCOUNT) && (0 < ACC_OPENMP_STREAM_MAXCOUNT)
  int i = 0;
# if defined(_OPENMP)
# pragma omp critical
# endif
  for (; i < ACC_OPENMP_STREAM_MAXCOUNT; ++i) {
    acc_openmp_streams[i].status = EXIT_SUCCESS;
  }
#endif
  return EXIT_SUCCESS;
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
{ /* Blocks the host-thread. */
  int result;
  acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
#if defined(ACC_OPENMP_STREAM_MAXCOUNT) && (0 < ACC_OPENMP_STREAM_MAXCOUNT)
  assert(NULL == s || (acc_openmp_streams <= s && s < (acc_openmp_streams + ACC_OPENMP_STREAM_MAXCOUNT)));
#endif
  if (NULL != stream && 0 < s->pending) {
    result = EXIT_SUCCESS; /* TODO */
  }
  else {
    result = EXIT_SUCCESS;
  }
  return result;
}


int acc_stream_wait_event(acc_stream_t stream, acc_event_t event)
{ /* Waits (device-side) for an event (potentially recorded on a different stream). */
  int result;
  if (NULL != stream && NULL != event) {
#if !defined(ACC_OPENMP_OFFLOAD)
    (void)(stream); /* unused */
#else /* implies _OPENMP */
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        deps->args[0].const_ptr = event;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          int tid = 0;
          for (; tid < nthreads; ++tid) {
            const acc_openmp_depend_t *const di = &deps[tid];
            const acc_openmp_event_t *const ei = (const acc_openmp_event_t*)di->args[0].const_ptr;
            const char *const ie = ei->dependency;
            if (NULL != ie) { /* still pending */
              const char *const id = di->in, *const od = di->out;
              (void)(id); (void)(od); /* suppress incorrect warning */
#             pragma omp target depend(in:id[0],ie[0]) depend(out:od[0]) nowait if(0)
              {}
            }
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    result = acc_event_synchronize(event);
  }
  else result = (NULL == event ? EXIT_SUCCESS : EXIT_FAILURE);
  return result;
}

#if defined(__cplusplus)
}
#endif
