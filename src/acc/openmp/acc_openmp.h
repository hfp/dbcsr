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

#if !defined(ACC_OPENMP_STRING_MAXLEN)
# define ACC_OPENMP_STRING_MAXLEN 32
#endif
#if !defined(ACC_OPENMP_STREAM_MAXCOUNT)
# define ACC_OPENMP_STREAM_MAXCOUNT 16
#endif
#if !defined(ACC_OPENMP_EVENT_MAXCOUNT)
# define ACC_OPENMP_EVENT_MAXCOUNT (16*ACC_OPENMP_STREAM_MAXCOUNT)
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

ACC_OPENMP_EXPORT typedef struct acc_openmp_stream_t {
  char name[ACC_OPENMP_STRING_MAXLEN];
  int priority;
} acc_openmp_stream_t;

ACC_OPENMP_EXPORT typedef struct acc_openmp_event_t {
  int dummy;
} acc_openmp_event_t;

/** Helper function for lock-free allocation of preallocated items such as streams or events. */
ACC_OPENMP_EXPORT void* acc_openmp_alloc(int typesize, void* storage, void** pointer, int* counter, int maxcount);
/** Helper function for lock-free deallocation (companion of acc_openmp_alloc). */
ACC_OPENMP_EXPORT int acc_openmp_dealloc(void* item, int typesize, void* storage, void** pointer, int* counter, int maxcount);
/** OpenMP does not distinct between H2H, H2D, D2H, or D2D. This common form is used for OpenMP backend. */
ACC_OPENMP_EXPORT int acc_memcpy(const void* src, void* dst, size_t size, acc_stream_t stream);

#endif /*ACC_OPENMP_H*/
