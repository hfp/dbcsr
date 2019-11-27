/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "dbcsr_omp.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#if defined(__cplusplus)
extern "C" {
#endif

dbcsr_omp_depend_t dbcsr_omp_stream_call[DBCSR_OMP_THREADS_MAXCOUNT];
#if defined(DBCSR_OMP_STREAM_MAXCOUNT) && (0 < DBCSR_OMP_STREAM_MAXCOUNT)
dbcsr_omp_stream_t  dbcsr_omp_streams[DBCSR_OMP_STREAM_MAXCOUNT];
dbcsr_omp_stream_t* dbcsr_omp_streamp[DBCSR_OMP_STREAM_MAXCOUNT];
#endif
volatile int dbcsr_omp_stream_depend_count;
int dbcsr_omp_stream_count;


int dbcsr_omp_stream_depend(acc_stream_t* stream, dbcsr_omp_depend_t** depend)
{
  int result;
  dbcsr_omp_stream_t *const s = (dbcsr_omp_stream_t*)stream;
#if defined(DBCSR_OMP_STREAM_MAXCOUNT) && (0 < DBCSR_OMP_STREAM_MAXCOUNT)
  assert(NULL == s || (dbcsr_omp_streams <= s && s < (dbcsr_omp_streams + DBCSR_OMP_STREAM_MAXCOUNT)));
#endif
#if defined(DBCSR_OMP_OFFLOAD) && !defined(NDEBUG)
  assert(omp_get_default_device() == s->device_id);
#endif
  assert(NULL != depend);
  if (NULL != s && EXIT_SUCCESS == s->status) {
    static const dbcsr_omp_dependency_t dummy = 0;
    int index;
#if !defined(_OPENMP)
    dbcsr_omp_depend_t *const d = dbcsr_omp_stream_call;
#else
    dbcsr_omp_depend_t *const d = &dbcsr_omp_stream_call[omp_get_thread_num()];
# if defined(_OPENMP) && (200805 <= _OPENMP) /* OpenMP 3.0 */
#   pragma omp atomic capture
# elif defined(_OPENMP)
#   pragma omp critical(dbcsr_omp_stream_depend_critical)
# endif
#endif
    index = s->pending++;
    assert(NULL != d);
#if !defined(NDEBUG)
    memset(d->args, 0, DBCSR_OMP_ARGUMENTS_MAXCOUNT * sizeof(dbcsr_omp_any_t));
#endif
    d->out = s->name + index % DBCSR_OMP_STREAM_MAXPENDING;
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
  DBCSR_OMP_RETURN(result);
}


DBCSR_OMP_EXPORT void dbcsr_omp_stream_depend_begin(void)
{
#if defined(_OPENMP)
# pragma omp atomic
#endif
  ++dbcsr_omp_stream_depend_count;
}


int dbcsr_omp_stream_depend_nthreads(void)
{
#if defined(_OPENMP)
  const int result = omp_get_num_threads();
#else
  const int result = 1;
#endif
  int npause = 1;
  DBCSR_OMP_WAIT(result > dbcsr_omp_stream_depend_count, npause);
  return result;
}


int dbcsr_omp_stream_depend_end(void)
{
#if defined(_OPENMP)
  const int nthreads = omp_get_num_threads(), tid = omp_get_thread_num();
#else
  const int nthreads = 1, tid = 0;
#endif
  if (0 != tid) {
    int npause = 1;
    DBCSR_OMP_WAIT(dbcsr_omp_stream_depend_count < nthreads, npause);
  }
  else { /* master thread */
#if defined(_OPENMP)
#   pragma omp atomic
#endif
    dbcsr_omp_stream_depend_count -= nthreads;
  }
  DBCSR_OMP_RETURN(0 == dbcsr_omp_stream_depend_count ? EXIT_SUCCESS : EXIT_FAILURE);
}


int acc_stream_create(acc_stream_t** stream_p, const char* name, int priority)
{
  dbcsr_omp_stream_t* stream;
  const int result = dbcsr_omp_alloc((void**)&stream,
    sizeof(dbcsr_omp_stream_t), &dbcsr_omp_stream_count,
#if defined(DBCSR_OMP_STREAM_MAXCOUNT) && (0 < DBCSR_OMP_STREAM_MAXCOUNT)
    DBCSR_OMP_STREAM_MAXCOUNT, dbcsr_omp_streams, (void**)dbcsr_omp_streamp);
#else
    0, NULL, NULL);
#endif
  assert(NULL != stream_p);
  if (EXIT_SUCCESS == result) {
    assert(NULL != stream);
    strncpy(stream->name, name, DBCSR_OMP_STREAM_MAXPENDING);
    stream->name[DBCSR_OMP_STREAM_MAXPENDING-1] = '\0';
    stream->priority = priority;
    stream->pending = 0;
    stream->status = 0;
#if defined(DBCSR_OMP_OFFLOAD) && !defined(NDEBUG)
    stream->device_id = omp_get_default_device();
#endif
    *stream_p = stream;
  }
  DBCSR_OMP_RETURN(result);
}


int acc_stream_destroy(acc_stream_t* stream)
{
  dbcsr_omp_stream_t *const s = (dbcsr_omp_stream_t*)stream;
  int result = ((NULL != s && 0 < s->pending) ? acc_stream_sync(stream) : EXIT_SUCCESS);
#if defined(DBCSR_OMP_STREAM_MAXCOUNT) && (0 < DBCSR_OMP_STREAM_MAXCOUNT)
  assert(NULL == s || (dbcsr_omp_streams <= s && s < (dbcsr_omp_streams + DBCSR_OMP_STREAM_MAXCOUNT)));
#endif
  if (EXIT_SUCCESS == result) {
    result = dbcsr_omp_dealloc(stream, sizeof(dbcsr_omp_stream_t), &dbcsr_omp_stream_count,
#if defined(DBCSR_OMP_STREAM_MAXCOUNT) && (0 < DBCSR_OMP_STREAM_MAXCOUNT)
      DBCSR_OMP_STREAM_MAXCOUNT, dbcsr_omp_streams, (void**)dbcsr_omp_streamp);
#else
      0, NULL, NULL);
#endif
  }
  DBCSR_OMP_RETURN(result);
}


void dbcsr_omp_stream_clear_errors(void)
{
#if defined(_OPENMP)
# pragma omp critical
#endif
  {
#if defined(DBCSR_OMP_STREAM_MAXCOUNT) && (0 < DBCSR_OMP_STREAM_MAXCOUNT)
    int i = 0;
    for (; i < DBCSR_OMP_STREAM_MAXCOUNT; ++i) {
      dbcsr_omp_streams[i].status = EXIT_SUCCESS;
    }
#endif
    dbcsr_omp_stream_depend_count = 0; /* reset number of active dependencies */
  }
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
  DBCSR_OMP_RETURN(result);
}


int acc_stream_sync(acc_stream_t* stream)
{ /* Blocks the host-thread. */
  int result = (NULL != stream ? EXIT_SUCCESS : EXIT_FAILURE), npause = 1;
  if (EXIT_SUCCESS == result) {
    result = acc_event_record(NULL/*event*/, stream);
    if (EXIT_SUCCESS == result) {
      dbcsr_omp_stream_t* const s = (dbcsr_omp_stream_t*)stream;
      DBCSR_OMP_WAIT(s->pending, npause);
    }
  }
  DBCSR_OMP_RETURN(result);
}


int acc_stream_wait_event(acc_stream_t* stream, acc_event_t* event)
{ /* Waits (device-side) for an event (potentially recorded on a different stream). */
  int result;
  if (NULL != stream && NULL != event) {
#if defined(DBCSR_OMP_OFFLOAD)
    if (0 < dbcsr_omp_ndevices()) {
      dbcsr_omp_depend_t* deps;
      result = dbcsr_omp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        deps->args[0].const_ptr = event;
      }
      dbcsr_omp_stream_depend_begin();
#     pragma omp master
      { const int nthreads = dbcsr_omp_stream_depend_nthreads();
        int tid = 0;
        for (; tid < nthreads; ++tid) {
          const dbcsr_omp_depend_t *const di = &deps[tid];
          const dbcsr_omp_event_t *const ei = (const dbcsr_omp_event_t*)di->args[0].const_ptr;
          const char *const ie = ei->dependency;
          if (NULL != ie) { /* still pending */
            const char *const id = di->in, *const od = di->out;
            (void)(id); (void)(od); /* suppress incorrect warning */
#           pragma omp target depend(in:DBCSR_OMP_DEP(id),DBCSR_OMP_DEP(ie)) depend(out:DBCSR_OMP_DEP(od)) nowait if(0)
            {}
          }
        }
      }
      result = dbcsr_omp_stream_depend_end();
    }
    else
#endif
    result = acc_event_synchronize(event);
  }
  else result = (NULL == event ? EXIT_SUCCESS : EXIT_FAILURE);
  DBCSR_OMP_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
