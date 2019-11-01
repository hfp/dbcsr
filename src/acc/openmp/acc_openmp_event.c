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
  int result = acc_openmp_alloc((void**)&event,
    sizeof(acc_openmp_event_t), &acc_openmp_event_count,
#if defined(ACC_OPENMP_EVENT_MAXCOUNT) && (0 < ACC_OPENMP_EVENT_MAXCOUNT)
    ACC_OPENMP_EVENT_MAXCOUNT, acc_openmp_events, (void**)acc_openmp_eventp);
#else
    0, NULL, NULL);
#endif
  if (EXIT_SUCCESS == result) {
    event->has_occurred = 0;
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
#if defined(ACC_OPENMP)
  result = EXIT_FAILURE; /* TODO */
#else
  (void)(event); (void)(stream); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_event_query(acc_event_t event, int* has_occurred)
{
  int result;
#if defined(ACC_OPENMP)
  result = EXIT_FAILURE; /* TODO */
#else
  (void)(event); (void)(has_occurred); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_event_synchronize(acc_event_t event)
{
  int result;
#if defined(ACC_OPENMP)
  result = EXIT_FAILURE; /* TODO */
#else
  (void)(event); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
}

#if defined(__cplusplus)
}
#endif
