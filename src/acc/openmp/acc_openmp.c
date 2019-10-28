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

#if !defined(ACC_OPENMP_MALLOC) && 0
# define ACC_OPENMP_MALLOC
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int acc_openmp_initialized;


void* acc_openmp_alloc(int typesize, void* storage, void** pointer, int* counter, int maxcount)
{
  void* result;
  assert(NULL != storage && NULL != pointer && NULL != counter && 0 < maxcount); /* no runtime check */
#if defined(ACC_OPENMP_MALLOC)
  result = malloc(typesize);
#else /* fast allocation */
  { int i;
# if defined(ACC_OPENMP)
#   pragma omp atomic capture
# endif
    i = (*counter)++;
    if (i < maxcount) {
      result = pointer[i];
      if (NULL == result) {
        result = (char*)storage + i * typesize;
      }
    }
    else { /* out of space */
# if defined(ACC_OPENMP)
#     pragma omp atomic
# endif
      --*counter;
      result = NULL;
    }
  }
#endif
  assert(0 < acc_openmp_initialized);
  return result;
}


int acc_openmp_dealloc(void* item, int typesize, void* storage, void** pointer, int* counter, int maxcount)
{
  int result;
  assert(NULL != storage && NULL != pointer && NULL != counter && 0 < maxcount); /* no runtime check */
#if defined(ACC_OPENMP_MALLOC)
  free(item);
#else /* fast allocation */
  if (NULL != item) {
    int i;
# if defined(ACC_OPENMP)
#   pragma omp atomic capture
# endif
    i = (*counter)--;
    if (0 <= i && i < maxcount && storage <= item /* check if item came from storage */
      && ((const char*)item) < ((const char*)storage + maxcount * typesize))
    {
      result = EXIT_SUCCESS;
      pointer[i] = item;
    }
    else { /* invalid free */
      result = EXIT_FAILURE;
# if defined(ACC_OPENMP)
#     pragma omp atomic
# endif
      ++*counter;
    }
  }
  else
#endif
  {
    result = EXIT_SUCCESS;
  }
  assert(0 < acc_openmp_initialized);
  return result;
}


int acc_init(void)
{
#if defined(ACC_OPENMP)
# pragma omp atomic
#endif
  ++acc_openmp_initialized;
  return 1 == acc_openmp_initialized
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}


int acc_finalize(void)
{
#if defined(ACC_OPENMP)
# pragma omp atomic
#endif
  --acc_openmp_initialized;
  return 0 == acc_openmp_initialized
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}


int acc_clear_errors(void)
{
  return (0 < acc_openmp_initialized ? EXIT_SUCCESS : EXIT_FAILURE);
}


int acc_get_ndevices(int* n_devices)
{
  int result;
#if defined(ACC_OPENMP)
# pragma omp single
#endif
  if (NULL != n_devices) {
#if defined(ACC_OPENMP) && (!defined(ACC_OPENMP_DEVICE_MAXCOUNT) || (0 != ACC_OPENMP_DEVICE_MAXCOUNT))
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
  int result;
#if !defined(ACC_OPENMP)
  (void)(device_id); /* unused */
#else
# pragma omp master
  if (0 <= device_id) {
# if !defined(NDEBUG)
    int ndevices;
# if !defined(ACC_OPENMP_DEVICE_MAXCOUNT) || (0 != ACC_OPENMP_DEVICE_MAXCOUNT)
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
# endif
    {
# if !defined(ACC_OPENMP_DEVICE_MAXCOUNT) || (0 != ACC_OPENMP_DEVICE_MAXCOUNT)
      omp_set_default_device(device_id);
# endif
      result = EXIT_SUCCESS;
    }
# if !defined(NDEBUG)
    else {
      result = EXIT_FAILURE;
    }
# endif
  }
  else
#endif
  {
    result = EXIT_FAILURE;
  }
  assert(0 < acc_openmp_initialized);
  return result;
}

#if defined(__cplusplus)
}
#endif
