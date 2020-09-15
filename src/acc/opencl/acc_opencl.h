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
# if defined(__APPLE__)
#   include <OpenCL/cl.h>
# else
#   include <CL/cl.h>
# endif
#else
# error Definition of __OPENCL preprocessor symbol is missing!
#endif

#if !defined(ACC_OPENCL_NOEXT)
# if defined(__APPLE__)
#   include <OpenCL/cl_ext.h>
# else
#   include <CL/cl_ext.h>
# endif
#endif

#if !defined(ACC_OPENCL_THREADLOCAL_CONTEXTACC_OPENCL_THREADLOCAL_CONTEXT)
# define ACC_OPENCL_THREADLOCAL_CONTEXT
#endif
#if !defined(ACC_OPENCL_CACHELINE_NBYTES)
# define ACC_OPENCL_CACHELINE_NBYTES 64
#endif
#if !defined(ACC_OPENCL_STRING_MAXLENGTH)
# define ACC_OPENCL_STRING_MAXLENGTH 256
#endif
#if !defined(ACC_OPENCL_BUFFER_MAXSIZE)
# define ACC_OPENCL_BUFFER_MAXSIZE (8 << 10/*8kB*/)
#endif
#if !defined(ACC_OPENCL_DEVICES_MAXCOUNT)
# define ACC_OPENCL_DEVICES_MAXCOUNT 32
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
#define ACC_OPENCL_UNUSED(VAR) (void)(VAR)

#if defined(NDEBUG)
# define ACC_OPENCL_EXPECT(EXPECTED, EXPR) EXPR
# define ACC_OPENCL_RETURN(RESULT) return RESULT
# define ACC_OPENCL_ERROR(MSG, RESULT) (RESULT) = EXIT_FAILURE
#else
# define ACC_OPENCL_EXPECT(EXPECTED, EXPR) assert((EXPECTED) == (EXPR))
# define ACC_OPENCL_RETURN(RESULT) do { \
    const int acc_opencl_return_result_ = (RESULT); \
    assert(EXIT_SUCCESS == acc_opencl_return_result_); \
    return acc_opencl_return_result_; \
  } while (0)
# define ACC_OPENCL_ERROR(MSG, RESULT) do { \
    if (-1001 != (RESULT)) { \
      fprintf(stderr, "ERROR ACC/OpenCL: " MSG " (code=%i)\n", RESULT); \
      assert(CL_SUCCESS != (RESULT)); \
    } \
    else { \
      fprintf(stderr, "ERROR ACC/OpenCL: incomplete OpenCL installation (" MSG ")\n"); \
    } \
    assert(!MSG); \
    (RESULT) = EXIT_FAILURE; \
  } while (0)
#endif

#define ACC_OPENCL_CHECK(EXPR, MSG, RESULT) do { \
  if (EXIT_SUCCESS == (RESULT)) { \
    (RESULT) = (EXPR); assert((MSG) && *(MSG)); \
    if (CL_SUCCESS == (RESULT)) { \
      (RESULT) = EXIT_SUCCESS; \
    } \
    else { \
      ACC_OPENCL_ERROR(MSG, RESULT); \
    } \
  } \
} while (0)

#if defined(__cplusplus)
extern "C" {
#endif

struct acc_stream_t {
#if defined(ACC_OPENCL_STRING_MAXLENGTH) && (0 < ACC_OPENCL_STRING_MAXLENGTH) && !defined(NDEBUG)
  char name[ACC_OPENCL_STRING_MAXLENGTH];
#endif
  cl_command_queue queue;
};

struct acc_event_t {
  cl_event event;
};

/* allow a context per each OpenMP thread */
extern cl_context acc_opencl_context;
#if defined(_OPENMP) && defined(ACC_OPENCL_THREADLOCAL_CONTEXT)
# pragma omp threadprivate(acc_opencl_context)
#endif

extern volatile int acc_opencl_stream_count;
extern volatile int acc_opencl_event_count;
/* non-zero if library is initialized, zero devices is signaled by nagative value */
extern int acc_opencl_ndevices;

/** Helper function for lock-free allocation of preallocated items such as streams or events. */
int acc_opencl_alloc(void** item, size_t typesize, volatile int* counter, int maxcount, void* storage, void** pointer);
/** Helper function for lock-free deallocation (companion of acc_opencl_alloc). */
int acc_opencl_dealloc(void* item, size_t typesize, volatile int* counter, int maxcount, void* storage, void** pointer);
/** Returns the pointer to the 1st match of "b" in "a". */
const char* acc_opencl_stristr(const char* a, const char* b);
/** Clears status of all streams (if possible). */
void acc_opencl_stream_clear_errors(void);

#if defined(__cplusplus)
}
#endif

#endif /*ACC_OPENCL_H*/
