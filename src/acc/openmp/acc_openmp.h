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

#if !defined(ACC_OPENMP_THREADS_MAXCOUNT)
# define ACC_OPENMP_THREADS_MAXCOUNT 8192
#endif
#if !defined(ACC_OPENMP_CACHELINE_NBYTES)
# define ACC_OPENMP_CACHELINE_NBYTES 64
#endif
#if !defined(ACC_OPENMP_ARGUMENTS_MAXCOUNT)
# define ACC_OPENMP_ARGUMENTS_MAXCOUNT 14
#endif
#if !defined(ACC_OPENMP_STREAM_MAXPENDING)
# define ACC_OPENMP_STREAM_MAXPENDING 256
#endif
#if !defined(ACC_OPENMP_STREAM_MAXCOUNT)
# define ACC_OPENMP_STREAM_MAXCOUNT 16
#endif
#if !defined(ACC_OPENMP_EVENT_MAXCOUNT)
# define ACC_OPENMP_EVENT_MAXCOUNT (16*ACC_OPENMP_STREAM_MAXCOUNT)
#endif
#if !defined(ACC_OPENMP_PAUSE_MAXCOUNT)
# define ACC_OPENMP_PAUSE_MAXCOUNT 4096
#endif
#if !defined(ACC_OPENMP_DEVICE_MAXCOUNT) && 0
# define ACC_OPENMP_DEVICE_MAXCOUNT 0
#endif

#if defined(__INTEL_COMPILER)
# if !defined(__INTEL_COMPILER_UPDATE)
#   define ACC_OPENMP_INTEL_COMPILER __INTEL_COMPILER
# else
#   define ACC_OPENMP_INTEL_COMPILER (__INTEL_COMPILER + __INTEL_COMPILER_UPDATE)
# endif
#elif defined(__INTEL_COMPILER_BUILD_DATE)
# define ACC_OPENMP_INTEL_COMPILER ((__INTEL_COMPILER_BUILD_DATE / 10000 - 2000) * 100)
#endif

#if defined(__STDC_VERSION__) && (199901L <= __STDC_VERSION__) /*C99*/
# define ACC_OPENMP_PRAGMA(DIRECTIVE) _Pragma(ACC_OPENMP_STRINGIFY(DIRECTIVE))
#else
# if defined(ACC_OPENMP_INTEL_COMPILER) || defined(_MSC_VER)
#   define ACC_OPENMP_PRAGMA(DIRECTIVE) __pragma(ACC_OPENMP_EXPAND(DIRECTIVE))
# else
#   define ACC_OPENMP_PRAGMA(DIRECTIVE)
# endif
#endif

#if (defined(__GNUC__) && ( \
    (defined(__x86_64__) && 0 != (__x86_64__)) || \
    (defined(__amd64__) && 0 != (__amd64__)) || \
    (defined(_M_X64) || defined(_M_AMD64)) || \
    (defined(__i386__) && 0 != (__i386__)) || \
    (defined(_M_IX86))))
# define ACC_OPENMP_PAUSE __asm__ __volatile__("pause" ::: "memory")
#else
# define ACC_OPENMP_PAUSE
#endif
#define ACC_OPENMP_WAIT(CONDITION, COUNTER) do { \
  while (CONDITION) { int counter = 0; \
    for (; counter <= (COUNTER); ++counter) ACC_OPENMP_PAUSE; \
    if ((COUNTER) < ACC_OPENMP_PAUSE_MAXCOUNT) { \
      (COUNTER) = 2 * (0 < (COUNTER) ? (COUNTER) : 1); \
    } else { /* yield? */ \
      (COUNTER) = ACC_OPENMP_PAUSE_MAXCOUNT; \
    } \
  } \
} while (CONDITION)

#if defined(__cplusplus)
# define ACC_OPENMP_EXTERN extern "C"
# define ACC_OPENMP_EXPORT ACC_OPENMP_EXTERN
#else
# define ACC_OPENMP_EXTERN extern
# define ACC_OPENMP_EXPORT
#endif

#if !defined(ACC_OPENMP_BASELINE)
# define ACC_OPENMP_BASELINE 45
#endif

#if defined(_OPENMP)
# if   (201811 <= _OPENMP/*v5.0*/)
#   define ACC_OPENMP_VERSION 50
# elif (201511 <= _OPENMP/*v4.5*/)
#   define ACC_OPENMP_VERSION 45
# elif (201307 <= _OPENMP/*v4.0*/)
#   define ACC_OPENMP_VERSION 40
# endif
# if defined(ACC_OPENMP_VERSION) && (ACC_OPENMP_BASELINE <= ACC_OPENMP_VERSION)
#   define ACC_OPENMP_OFFLOAD
# elif !defined(ACC_OPENMP_VERSION) && defined(__ibmxl__)
#   define ACC_OPENMP_VERSION ACC_OPENMP_BASELINE
#   define ACC_OPENMP_OFFLOAD
# endif
#endif

#include "../include/acc.h"
#include <stdint.h>
#if defined(_OPENMP)
# include <omp.h>
#endif

#define ACC_OPENMP_EXPAND(SYMBOL) SYMBOL
#define ACC_OPENMP_STRINGIFY2(SYMBOL) #SYMBOL
#define ACC_OPENMP_STRINGIFY(SYMBOL) ACC_OPENMP_STRINGIFY2(SYMBOL)
#define ACC_OPENMP_UP2(N, NPOT) ((((uint64_t)N) + ((NPOT) - 1)) & ~((NPOT) - 1))


ACC_OPENMP_EXPORT typedef struct acc_openmp_stream_t {
  /* address of each character is (side-)used to form OpenMP task dependencies */
  char name[ACC_OPENMP_STREAM_MAXPENDING];
  volatile int pending, status;
  int priority;
#if defined(ACC_OPENMP_OFFLOAD) && !defined(NDEBUG)
  int device_id; /* should match active device as set by acc_set_active_device */
#endif
} acc_openmp_stream_t;

ACC_OPENMP_EXPORT typedef struct acc_openmp_event_t {
  const char *volatile dependency;
} acc_openmp_event_t;

ACC_OPENMP_EXPORT typedef union acc_openmp_any_t {
  const void* const_ptr; void* ptr;
  uint64_t u64;
  int64_t i64;
  size_t size;
  uint32_t u32;
  int32_t i32;
  acc_bool_t logical;
} acc_openmp_any_t;

ACC_OPENMP_EXPORT typedef struct acc_openmp_depend_t {
  char data[ACC_OPENMP_UP2(ACC_OPENMP_ARGUMENTS_MAXCOUNT*sizeof(acc_openmp_any_t)+2*sizeof(void*),ACC_OPENMP_CACHELINE_NBYTES)];
  /** Used to record the arguments/signature of each OpenMP-offload/call on a per-thread basis. */
  acc_openmp_any_t args[ACC_OPENMP_ARGUMENTS_MAXCOUNT];
  /** The in/out-pointer must be dereferenced (depend clause expects value; due to syntax issues use in[0]/out[0]). */
  const char *in, *out;
} acc_openmp_depend_t;

/** Helper function for lock-free allocation of preallocated items such as streams or events. */
ACC_OPENMP_EXPORT int acc_openmp_alloc(void** item, int typesize, int* counter, int maxcount, void* storage, void** pointer);
/** Helper function for lock-free deallocation (companion of acc_openmp_alloc). */
ACC_OPENMP_EXPORT int acc_openmp_dealloc(void* item, int typesize, int* counter, int maxcount, void* storage, void** pointer);
/** Generate dependency for given stream. */
ACC_OPENMP_EXPORT int acc_openmp_stream_depend(acc_stream_t* stream, acc_openmp_depend_t** depend);
/** Clears status of all streams (if possible). */
ACC_OPENMP_EXPORT int acc_openmp_stream_clear_errors(void);

#endif /*ACC_OPENMP_H*/
