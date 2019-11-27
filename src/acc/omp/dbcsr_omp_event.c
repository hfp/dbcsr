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

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(DBCSR_OMP_EVENT_MAXCOUNT) && (0 < DBCSR_OMP_EVENT_MAXCOUNT)
dbcsr_omp_event_t  dbcsr_omp_events[DBCSR_OMP_EVENT_MAXCOUNT];
dbcsr_omp_event_t* dbcsr_omp_eventp[DBCSR_OMP_EVENT_MAXCOUNT];
#endif
int dbcsr_omp_event_count;


int acc_event_create(acc_event_t** event_p)
{
  dbcsr_omp_event_t* event;
  const int result = dbcsr_omp_alloc((void**)&event,
    sizeof(dbcsr_omp_event_t), &dbcsr_omp_event_count,
#if defined(DBCSR_OMP_EVENT_MAXCOUNT) && (0 < DBCSR_OMP_EVENT_MAXCOUNT)
    DBCSR_OMP_EVENT_MAXCOUNT, dbcsr_omp_events, (void**)dbcsr_omp_eventp);
#else
    0, NULL, NULL);
#endif
  if (EXIT_SUCCESS == result) {
    event->dependency = NULL;
    *event_p = event;
  }
  DBCSR_OMP_RETURN(result);
}


int acc_event_destroy(acc_event_t* event)
{
  return dbcsr_omp_dealloc(event, sizeof(dbcsr_omp_event_t), &dbcsr_omp_event_count,
#if defined(DBCSR_OMP_EVENT_MAXCOUNT) && (0 < DBCSR_OMP_EVENT_MAXCOUNT)
    DBCSR_OMP_EVENT_MAXCOUNT, dbcsr_omp_events, (void**)dbcsr_omp_eventp);
#else
    0, NULL, NULL);
#endif
}


int acc_event_record(acc_event_t* event, acc_stream_t* stream)
{
  int result;
  if (NULL != stream) {
    dbcsr_omp_stream_t *const s = (dbcsr_omp_stream_t*)stream;
    dbcsr_omp_event_t *const e = (dbcsr_omp_event_t*)event;
#if defined(DBCSR_OMP_OFFLOAD)
    if (0 < dbcsr_omp_ndevices()) {
      dbcsr_omp_depend_t* deps;
      result = dbcsr_omp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        if (NULL != e) e->dependency = deps->out; /* reset if re-enqueued */
        deps->args[0].ptr = event;
      }
      dbcsr_omp_stream_depend_begin();
#     pragma omp master
      { const int nthreads = dbcsr_omp_stream_depend_nthreads();
        int tid = 0;
        for (; tid < nthreads; ++tid) {
          dbcsr_omp_depend_t *const di = &deps[tid];
          dbcsr_omp_event_t *const ei = (dbcsr_omp_event_t*)di->args[0].ptr;
          const char *const id = di->in, *const od = di->out;
          (void)(id); (void)(od); /* suppress incorrect warning */
          if (NULL != ei) {
            uintptr_t/*const char**/ volatile* /*const*/ sig = (uintptr_t volatile*)&ei->dependency;
#           pragma omp target depend(in:DBCSR_OMP_DEP(id)) depend(out:DBCSR_OMP_DEP(od)) nowait map(from:sig[0:1])
            *sig = 0/*NULL*/;
          }
          else {
            int volatile* /*const*/ sig = (int volatile*)&s->pending;
#           pragma omp target depend(in:DBCSR_OMP_DEP(id)) depend(out:DBCSR_OMP_DEP(od)) nowait map(from:sig[0:1])
            *sig = 0;
          }
        }
      }
      result = dbcsr_omp_stream_depend_end();
    }
    else
#endif
    if (NULL != e) {
      e->dependency = NULL;
      result = EXIT_SUCCESS;
    }
    else {
      s->pending = 0;
      result = EXIT_SUCCESS;
    }
  }
  else if (NULL == event) { /* flush all pending work */
    result = EXIT_SUCCESS;
#if defined(DBCSR_OMP_OFFLOAD) && 0
#   pragma omp master
#   pragma omp task if(0)
    result = EXIT_FAILURE;
#elif defined(_OPENMP)
#   pragma omp barrier
#endif
  }
  else result = EXIT_FAILURE;
  DBCSR_OMP_RETURN(result);
}


int acc_event_query(acc_event_t* event, acc_bool_t* has_occurred)
{
  int result = EXIT_FAILURE;
  if (NULL != has_occurred) {
    if (NULL != event) {
      const dbcsr_omp_event_t *const e = (dbcsr_omp_event_t*)event;
      *has_occurred = (NULL == e->dependency);
      result = EXIT_SUCCESS;
    }
    else *has_occurred = 0;
  }
  DBCSR_OMP_RETURN(result);
}


int acc_event_synchronize(acc_event_t* event)
{ /* Waits on the host-side. */
  int result;
  if (NULL != event) {
    const dbcsr_omp_event_t *const e = (dbcsr_omp_event_t*)event;
    int npause = 1;
    DBCSR_OMP_WAIT(NULL != e->dependency, npause);
    result = EXIT_SUCCESS;
  }
  else result = EXIT_FAILURE;
  DBCSR_OMP_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
