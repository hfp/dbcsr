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
#include <assert.h>


#if defined(__cplusplus)
extern "C" {
#endif

int acc_openmp_initialized;


int acc_openmp_alloc(void** item, int typesize, int* counter, int maxcount, void* storage, void** pointer)
{
  int result, i;
  assert(0 < acc_openmp_initialized && NULL != item && 0 < typesize && NULL != counter);
#if defined(_OPENMP) && (200805 <= _OPENMP) /* OpenMP 3.0 */
# pragma omp atomic capture
#else
# pragma omp critical(acc_openmp_alloc_critical)
#endif
  i = (*counter)++;
  if (0 < maxcount) { /* fast allocation */
    if (i < maxcount) {
      assert(NULL != storage && NULL != pointer);
      *item = pointer[i];
      if (NULL == *item) {
        *item = &((char*)storage)[i*typesize];
      }
      assert(((const char*)storage) <= ((const char*)*item) && ((const char*)*item) < &((const char*)storage)[maxcount*typesize]);
      result = EXIT_SUCCESS;
    }
    else { /* out of space */
      result = EXIT_FAILURE;
#if defined(_OPENMP)
#     pragma omp atomic
#endif
      --(*counter);
      *item = NULL;
    }
  }
  else {
    *item = malloc(typesize);
    if (NULL != *item) {
      result = EXIT_SUCCESS;
    }
    else {
      result = EXIT_FAILURE;
#if defined(_OPENMP)
#     pragma omp atomic
#endif
      --(*counter);
    }
  }
  return result;
}


int acc_openmp_dealloc(void* item, int typesize, int* counter, int maxcount, void* storage, void** pointer)
{
  int result;
  assert(0 < acc_openmp_initialized && 0 < typesize && NULL != counter);
  if (NULL != item) {
    int i;
#if defined(_OPENMP) && (200805 <= _OPENMP) /* OpenMP 3.0 */
#   pragma omp atomic capture
#else
#   pragma omp critical(acc_openmp_alloc_critical)
#endif
    i = (*counter)--;
    assert(0 <= i);
    if (0 < maxcount) { /* fast allocation */
      assert(NULL != storage && NULL != pointer);
      result = ((0 <= i && i < maxcount && storage <= item && /* check if item came from storage */
          ((const char*)item) < &((const char*)storage)[maxcount*typesize])
        ? EXIT_SUCCESS : EXIT_FAILURE);
      pointer[i] = item;
    }
    else {
      result = EXIT_SUCCESS;
      free(item);
    }
  }
  else {
    result = EXIT_SUCCESS;
  }
  return result;
}


int acc_init(void)
{
#if defined(_OPENMP)
# pragma omp atomic
#endif
  ++acc_openmp_initialized;
  return 1 == acc_openmp_initialized
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}


int acc_finalize(void)
{
  extern int acc_openmp_stream_count;
  extern int acc_openmp_event_count;
#if defined(_OPENMP)
# pragma omp atomic
#endif
  --acc_openmp_initialized;
  return (0 == acc_openmp_initialized
    && 0 == acc_openmp_stream_count
    && 0 == acc_openmp_event_count)
      ? EXIT_SUCCESS
      : EXIT_FAILURE;
}


int acc_clear_errors(void)
{
  return (0 < acc_openmp_initialized ? acc_openmp_stream_clear_errors() : EXIT_FAILURE);
}


int acc_get_ndevices(int* n_devices)
{
  int result;
  if (NULL != n_devices) {
#if defined(ACC_OPENMP_OFFLOAD) && (!defined(ACC_OPENMP_DEVICE_MAXCOUNT) || (0 != ACC_OPENMP_DEVICE_MAXCOUNT))
    *n_devices = omp_get_num_devices();
# if defined(ACC_OPENMP_DEVICE_MAXCOUNT) && (0 < ACC_OPENMP_DEVICE_MAXCOUNT)
    if (ACC_OPENMP_DEVICE_MAXCOUNT < *n_devices) {
      *n_devices = ACC_OPENMP_DEVICE_MAXCOUNT;
    }
# endif
#else
    *n_devices = 0;
#endif
    assert(0 <= *n_devices);
    result = EXIT_SUCCESS;
  }
  else {
    result = EXIT_FAILURE;
  }
  assert(0 < acc_openmp_initialized);
  return result;
}


int acc_set_active_device(int device_id)
{
  int result = (0 <= device_id ? EXIT_SUCCESS : EXIT_FAILURE);
#if defined(_OPENMP)
# pragma omp master
#endif
  if (EXIT_SUCCESS == result) {
#if !defined(NDEBUG)
    int ndevices;
# if defined(ACC_OPENMP_OFFLOAD) && (!defined(ACC_OPENMP_DEVICE_MAXCOUNT) || (0 != ACC_OPENMP_DEVICE_MAXCOUNT))
    ndevices = omp_get_num_devices();
#   if defined(ACC_OPENMP_DEVICE_MAXCOUNT) && (0 < ACC_OPENMP_DEVICE_MAXCOUNT)
    if (ACC_OPENMP_DEVICE_MAXCOUNT < ndevices) {
      ndevices = ACC_OPENMP_DEVICE_MAXCOUNT;
    }
#   endif
# else
    ndevices = 0;
# endif
    if (device_id < ndevices)
#endif
    {
#if defined(ACC_OPENMP_OFFLOAD) && (!defined(ACC_OPENMP_DEVICE_MAXCOUNT) || (0 != ACC_OPENMP_DEVICE_MAXCOUNT))
      omp_set_default_device(device_id);
#endif
      result = EXIT_SUCCESS;
    }
#if !defined(NDEBUG)
    else {
      result = EXIT_FAILURE;
    }
#endif
  }
  assert(0 < acc_openmp_initialized);
  return result;
}

#if defined(__cplusplus)
}
#endif
