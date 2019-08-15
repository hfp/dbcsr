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

#if defined(ACC_OPENMP)
acc_openmp_event_t  acc_openmp_events[ACC_OPENMP_EVENT_MAXCOUNT];
acc_openmp_event_t* acc_openmp_eventp[ACC_OPENMP_EVENT_MAXCOUNT];
int acc_openmp_event_count;
#endif


int acc_event_create(acc_event_t* event_p)
{
  int result;
#if !defined(ACC_OPENMP)
  (void)(event_p); /* unused */
#else
  acc_openmp_event_t *const event = (acc_openmp_event_t*)acc_openmp_alloc(
    sizeof(acc_openmp_event_t), acc_openmp_events, (void**)acc_openmp_eventp,
    &acc_openmp_event_count, ACC_OPENMP_EVENT_MAXCOUNT);
  if (NULL != event) {
    event->dummy = 0;
    *event_p = event;
    result = EXIT_SUCCESS;
  }
  else
#endif
  {
    result = EXIT_FAILURE;
  }
  return result;
}


int acc_event_destroy(acc_event_t event)
{
  int result;
#if defined(ACC_OPENMP)
  result = acc_openmp_dealloc(event,
    sizeof(acc_openmp_event_t), acc_openmp_events, (void**)acc_openmp_eventp,
    &acc_openmp_event_count, ACC_OPENMP_EVENT_MAXCOUNT);
#else
  (void)(event); /* unused */
  result = EXIT_FAILURE;
#endif
  return result;
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
