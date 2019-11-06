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

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(ACC_OPENMP_EVENT_MAXCOUNT) && (0 < ACC_OPENMP_EVENT_MAXCOUNT)
acc_openmp_event_t  acc_openmp_events[ACC_OPENMP_EVENT_MAXCOUNT];
acc_openmp_event_t* acc_openmp_eventp[ACC_OPENMP_EVENT_MAXCOUNT];
#endif
int acc_openmp_event_count;


int acc_event_create(acc_event_t* event_p)
{
  acc_openmp_event_t* event;
  const int result = acc_openmp_alloc((void**)&event,
    sizeof(acc_openmp_event_t), &acc_openmp_event_count,
#if defined(ACC_OPENMP_EVENT_MAXCOUNT) && (0 < ACC_OPENMP_EVENT_MAXCOUNT)
    ACC_OPENMP_EVENT_MAXCOUNT, acc_openmp_events, (void**)acc_openmp_eventp);
#else
    0, NULL, NULL);
#endif
  if (EXIT_SUCCESS == result) {
    event->dependency = NULL;
    *event_p = event;
  }
  return result;
}


int acc_event_destroy(acc_event_t event)
{
  return acc_openmp_dealloc(event, sizeof(acc_openmp_event_t), &acc_openmp_event_count,
#if defined(ACC_OPENMP_EVENT_MAXCOUNT) && (0 < ACC_OPENMP_EVENT_MAXCOUNT)
    ACC_OPENMP_EVENT_MAXCOUNT, acc_openmp_events, (void**)acc_openmp_eventp);
#else
    0, NULL, NULL);
#endif
}


int acc_event_record(acc_event_t event, acc_stream_t stream)
{
  int result;
  if (NULL != event && NULL != stream) {
    acc_openmp_event_t *const e = (acc_openmp_event_t*)event;
#if !defined(ACC_OPENMP_OFFLOAD)
    (void)(stream); /* unused */
#else /* implies _OPENMP */
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        e->dependency = deps->out; /* reset if re-enqueued */
        deps->args[0].ptr = event;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          int tid = 0;
          for (; tid < nthreads; ++tid) {
            acc_openmp_depend_t *const di = &deps[tid];
            acc_openmp_event_t *const ei = (acc_openmp_event_t*)di->args[0].ptr;
            const char *volatile *const has_occurred = &ei->dependency;
            const char *const id = di->in, *const od = di->out;
            (void)(id); (void)(od); /* suppress incorrect warning */
#           pragma omp target depend(in:id[0]) depend(out:od[0]) nowait map(from:has_occurred[0:1])
            *has_occurred = NULL;
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    {
      e->dependency = NULL;
      result = EXIT_SUCCESS;
    }
  }
  else result = (NULL == event ? EXIT_SUCCESS : EXIT_FAILURE);
  return result;
}


int acc_event_query(acc_event_t event, int* has_occurred)
{
  const int result = ((NULL != event && NULL != has_occurred) ? EXIT_SUCCESS : EXIT_FAILURE);
  if (EXIT_SUCCESS == result) {
    const acc_openmp_event_t *const e = (acc_openmp_event_t*)event;
    *has_occurred = (NULL == e->dependency);
  }
  return result;
}


int acc_event_synchronize(acc_event_t event)
{ /* Waits on the host-side. */
  int result;
  if (NULL != event) {
    const acc_openmp_event_t *const e = (const acc_openmp_event_t*)event;
    int npause = 1;
    while (NULL != e->dependency) {
      do {
        int counter = 0;
        for (; counter < npause; ++counter) ACC_OPENMP_PAUSE;
        if (npause < ACC_OPENMP_PAUSE_MAXCOUNT) {
          npause *= 2;
        }
        else {
          npause = ACC_OPENMP_PAUSE_MAXCOUNT;
          /* TODO: yield? */
        }
      } while (NULL != e->dependency);
    }
    result = EXIT_SUCCESS;
  }
  else result = EXIT_FAILURE;
  return result;
}

#if defined(__cplusplus)
}
#endif
