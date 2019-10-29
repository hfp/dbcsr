/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#ifndef ACC_OPENMP_H
#define ACC_OPENMP_H

#include "../include/acc.h"

#if !defined(ACC_OPENMP_STREAM_MAXPENDING)
# define ACC_OPENMP_STREAM_MAXPENDING 256
#endif
#if !defined(ACC_OPENMP_STREAM_MAXCOUNT)
# define ACC_OPENMP_STREAM_MAXCOUNT 16
#endif
#if !defined(ACC_OPENMP_EVENT_MAXCOUNT)
# define ACC_OPENMP_EVENT_MAXCOUNT (16*ACC_OPENMP_STREAM_MAXCOUNT)
#endif
#if !defined(ACC_OPENMP_DEVICE_MAXCOUNT) && 1
# define ACC_OPENMP_DEVICE_MAXCOUNT 0
#endif

#if defined(__cplusplus)
# define ACC_OPENMP_EXTERN extern "C"
# define ACC_OPENMP_EXPORT ACC_OPENMP_EXTERN
#else
# define ACC_OPENMP_EXTERN extern
# define ACC_OPENMP_EXPORT
#endif

#if defined(_OPENMP)
# if   (201811 <= _OPENMP/*v5.0*/)
#   define ACC_OPENMP_VERSION 50
# elif (201511 <= _OPENMP/*v4.5*/)
#   define ACC_OPENMP_VERSION 45
# elif (201307 <= _OPENMP/*v4.0*/)
#   define ACC_OPENMP_VERSION 40
# endif
# if defined(ACC_OPENMP_VERSION) && \
    (45 <= ACC_OPENMP_VERSION) /*baseline*/
#   define ACC_OPENMP
# endif
#endif

#if defined(ACC_OPENMP)
# include <omp.h>
#endif

#define ACC_OPENMP_DEPEND_IN(DEPEND) depend(in:DEPEND[0])
#define ACC_OPENMP_DEPEND_OUT(DEPEND) depend(out:DEPEND[0])


ACC_OPENMP_EXPORT typedef struct acc_openmp_stream_t {
  /* address of each character is (side-)used to form OpenMP task dependencies */
  char name[ACC_OPENMP_STREAM_MAXPENDING];
  int pending, priority, status;
#if defined(ACC_OPENMP) && !defined(NDEBUG)
  int device_id; /* should match active device as set by acc_set_active_device */
#endif
} acc_openmp_stream_t;

ACC_OPENMP_EXPORT typedef struct acc_openmp_event_t {
  int has_occurred;
} acc_openmp_event_t;

typedef const char* acc_openmp_depend_t;

/** Helper function for lock-free allocation of preallocated items such as streams or events. */
ACC_OPENMP_EXPORT void* acc_openmp_alloc(int typesize, void* storage, void** pointer, int* counter, int maxcount);
/** Helper function for lock-free deallocation (companion of acc_openmp_alloc). */
ACC_OPENMP_EXPORT int acc_openmp_dealloc(void* item, int typesize, void* storage, void** pointer, int* counter, int maxcount);
/** Generate dependency for given stream; in/out value must be dereferenced inside of depend-clause. */
ACC_OPENMP_EXPORT int acc_openmp_stream_depend(acc_stream_t stream, acc_openmp_depend_t* in, acc_openmp_depend_t* out);

#endif /*ACC_OPENMP_H*/
