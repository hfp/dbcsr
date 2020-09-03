/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#ifndef ACC_OPENCL_H
#define ACC_OPENCL_H

#include "../acc.h"
#if !defined(NDEBUG)
# include <assert.h>
#endif
#include <stdio.h>

#if defined(__OPENCL)
# include <CL/cl.h>
#else
# error Definition of OpenCL preprocessor symbol is missing!
#endif

#if !defined(ACC_OPENCL_CACHELINE_NBYTES)
# define ACC_OPENCL_CACHELINE_NBYTES 64
#endif
#if !defined(ACC_OPENCL_STRING_MAXLENGTH)
# define ACC_OPENCL_STRING_MAXLENGTH 256
#endif
#if !defined(ACC_OPENCL_STREAM_MAXCOUNT)
# define ACC_OPENCL_STREAM_MAXCOUNT 32
#endif
#if !defined(ACC_OPENCL_EVENT_MAXCOUNT)
# define ACC_OPENCL_EVENT_MAXCOUNT (32*(ACC_OPENCL_STREAM_MAXCOUNT))
#endif

#define ACC_OPENCL_EXPAND(SYMBOL) SYMBOL
#define ACC_OPENCL_STRINGIFY2(SYMBOL) #SYMBOL
#define ACC_OPENCL_STRINGIFY(SYMBOL) ACC_OPENCL_STRINGIFY2(SYMBOL)
#define ACC_OPENCL_UP2(N, NPOT) ((((uint64_t)N) + ((NPOT) - 1)) & ~((NPOT) - 1))

#if defined(NDEBUG)
# define ACC_OPENCL_EXPECT(EXPECTED, EXPR) EXPR
# define ACC_OPENCL_RETURN(RESULT) return RESULT
#else
# define ACC_OPENCL_EXPECT(EXPECTED, EXPR) assert((EXPECTED) == (EXPR))
# define ACC_OPENCL_RETURN(RESULT) do { \
    const int acc_opencl_return_result_ = (RESULT); \
    assert(EXIT_SUCCESS == acc_opencl_return_result_); \
    return acc_opencl_return_result_; \
  } while (0)
#endif

#define ACC_OPENCL_CHECK(EXPR, MSG) do { \
  const int acc_opencl_check_result_ = (EXPR); assert((MSG) && *(MSG)); \
  if (CL_SUCCESS != acc_opencl_check_result_) { \
    fprintf(stderr, "ERROR ACC/OpenCL: " MSG " (code=%i)\n", acc_opencl_check_result_); \
    assert(!MSG); \
  } \
} while (0)

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct acc_opencl_stream_t {
  /* address of each character is (side-)used to form OpenMP task dependencies */
  char name[ACC_OPENCL_STRING_MAXLENGTH];
  volatile int pending, status;
  int priority;
#if !defined(NDEBUG)
  int device_id; /* should match active device as set by acc_set_active_device */
#endif
} acc_opencl_stream_t;

extern int acc_opencl_stream_count;
extern int acc_opencl_event_count;
/* non-zero if library is initialized, zero devices is signaled by nagative value */
extern int acc_opencl_ndevices;
extern int acc_opencl_device;

/** Helper function for lock-free allocation of preallocated items such as streams or events. */
int acc_opencl_alloc(void** item, int typesize, int* counter, int maxcount, void* storage, void** pointer);
/** Helper function for lock-free deallocation (companion of acc_opencl_alloc). */
int acc_opencl_dealloc(void* item, int typesize, int* counter, int maxcount, void* storage, void** pointer);
/** Clears status of all streams (if possible). */
void acc_opencl_stream_clear_errors(void);

#if defined(__cplusplus)
}
#endif

#endif /*ACC_OPENCL_H*/
