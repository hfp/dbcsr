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

#if !defined(CL_TARGET_OPENCL_VERSION)
# define CL_TARGET_OPENCL_VERSION 220
#endif

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

#include "../acc.h"
#if !defined(NDEBUG)
# include <assert.h>
#endif
#include <stdio.h>

#if !defined(ACC_OPENCL_THREADLOCAL_CONTEXT)
# define ACC_OPENCL_THREADLOCAL_CONTEXT
#endif
#if !defined(ACC_OPENCL_CACHELINE_NBYTES)
# define ACC_OPENCL_CACHELINE_NBYTES 64
#endif
#if !defined(ACC_OPENCL_MAXALIGN_NBYTES)
# define ACC_OPENCL_MAXALIGN_NBYTES (2 << 20/*2MB*/)
#endif
#if !defined(ACC_OPENCL_BUFFER_MAXSIZE)
# define ACC_OPENCL_BUFFER_MAXSIZE (8 << 10/*8kB*/)
#endif
#if !defined(ACC_OPENCL_DEVICES_MAXCOUNT)
# define ACC_OPENCL_DEVICES_MAXCOUNT 32
#endif
#if !defined(ACC_OPENCL_MAXLINELEN)
# define ACC_OPENCL_MAXLINELEN 128
#endif

/* can depend on OpenCL implementation */
#if !defined(ACC_OPENCL_MEM_NOALLOC) && 1
# define ACC_OPENCL_MEM_NOALLOC
# define ACC_OPENCL_MEM(A) ((cl_mem*)&(A))
#else
# define ACC_OPENCL_MEM(A) ((cl_mem*)(A))
#endif
#if !defined(ACC_OPENCL_STREAM_NOALLOC) && 1
# define ACC_OPENCL_STREAM_NOALLOC
# define ACC_OPENCL_STREAM(A) ((cl_command_queue*)&(A))
#else
# define ACC_OPENCL_STREAM(A) ((cl_command_queue*)(A))
#endif
#if !defined(ACC_OPENCL_EVENT_NOALLOC) && 0
/* incompatible with acc_event_record */
# define ACC_OPENCL_EVENT_NOALLOC
# define ACC_OPENCL_EVENT(A) ((cl_event*)&(A))
#else
# define ACC_OPENCL_EVENT(A) ((cl_event*)(A))
#endif

#define ACC_OPENCL_EXPAND(SYMBOL) SYMBOL
#define ACC_OPENCL_STRINGIFY2(SYMBOL) #SYMBOL
#define ACC_OPENCL_STRINGIFY(SYMBOL) ACC_OPENCL_STRINGIFY2(SYMBOL)
#define ACC_OPENCL_UP2(N, NPOT) ((((uint64_t)N) + ((NPOT) - 1)) & ~((NPOT) - 1))
#define ACC_OPENCL_UNUSED(VAR) (void)(VAR)

#if defined(__STDC_VERSION__) && (199901L <= __STDC_VERSION__ || defined(__GNUC__))
# define ACC_OPENCL_SNPRINTF(S, N, ...) snprintf(S, N, __VA_ARGS__)
#else
# define ACC_OPENCL_SNPRINTF(S, N, ...) sprintf((S) + /*unused*/(N) * 0, __VA_ARGS__)
#endif

#if defined(_DEBUG)
# define ACC_OPENCL_DEBUG_PRINTF(A, ...) printf(A, __VA_ARGS__)
#else
# define ACC_OPENCL_DEBUG_PRINTF(A, ...)
#endif

#if defined(NDEBUG)
# define ACC_OPENCL_EXPECT(EXPECTED, EXPR) (EXPR)
# define ACC_OPENCL_ERROR(MSG, RESULT) (RESULT) = EXIT_FAILURE
# define ACC_OPENCL_RETURN_CAUSE(RESULT, CAUSE) ACC_OPENCL_UNUSED(CAUSE); return RESULT
#else
# define ACC_OPENCL_EXPECT(EXPECTED, EXPR) assert((EXPECTED) == (EXPR))
# define ACC_OPENCL_ERROR(MSG, RESULT) do { \
    if (-1001 != (RESULT)) { \
      fprintf(stderr, "ERROR ACC/OpenCL: " MSG " (code=%i)\n", RESULT); \
      assert(CL_SUCCESS != (RESULT)); \
    } \
    else { \
      fprintf(stderr, "ERROR ACC/OpenCL: incomplete installation (" MSG ")\n"); \
    } \
    assert(!MSG); \
    (RESULT) = EXIT_FAILURE; \
  } while (0)
# define ACC_OPENCL_RETURN_CAUSE(RESULT, CAUSE) do { \
    const int acc_opencl_return_cause_ = (RESULT); \
    if (EXIT_SUCCESS != acc_opencl_return_cause_) { \
      fprintf(stderr, "ERROR ACC/OpenCL: failed%s%s\n", \
        NULL == (CAUSE) ? "" : " for ", \
        NULL == (CAUSE) ? "" : ((const char*)(CAUSE))); \
      assert(!"NO ERROR"); \
    } \
    return acc_opencl_return_cause_; \
  } while (0)
#endif
#define ACC_OPENCL_RETURN(RESULT) ACC_OPENCL_RETURN_CAUSE(RESULT, NULL)

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

/* non-zero if library is initialized, zero devices is signaled by nagative value */
extern int acc_opencl_ndevices;
/* allow a context per each OpenMP thread */
extern cl_context acc_opencl_context;
#if defined(_OPENMP) && defined(ACC_OPENCL_THREADLOCAL_CONTEXT)
# pragma omp threadprivate(acc_opencl_context)
#endif

/**
 * Read source file or buffer[0] (if source is NULL), and build array of strings (buffer)
 * with line-wise content.The content is instantiated such that type replaces %s and
 * params replace %i (in order). Returns number processed lines, and if non-zero
 * buffer[0] shall be released by the caller (free).
 */
int acc_opencl_template(FILE* source, char* buffer[], int max_nlines, int skip_lines,
  const char* type, const int params[], int nparams);

#if defined(__cplusplus)
}
#endif

#endif /*ACC_OPENCL_H*/
