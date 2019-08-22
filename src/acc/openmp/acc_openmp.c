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
#if defined(ACC_OPENMP)
  int i;
  assert(NULL != storage && NULL != pointer && NULL != counter && 0 < maxcount); /* no runtime check */
# if defined(ACC_OPENMP_MALLOC)
  result = malloc(typesize);
# else /* fast allocation */
# pragma omp atomic capture
  i = (*counter)++;
  if (i < maxcount) {
    result = pointer[i];
    if (NULL == result) {
      result = (char*)storage + i * typesize;
    }
  }
  else { /* out of space */
#   pragma omp atomic
    --*counter;
    result = NULL;
  }
# endif
#else
  (void)(typesize); (void)(storage); (void)(pointer); (void)(counter); (void)(maxcount); /* unused */
  result = NULL;
#endif
  assert(0 < acc_openmp_initialized);
  return result;
}


int acc_openmp_dealloc(void* item, int typesize, void* storage, void** pointer, int* counter, int maxcount)
{
  int result;
#if defined(ACC_OPENMP)
  int i;
  assert(NULL != storage && NULL != pointer && NULL != counter && 0 < maxcount); /* no runtime check */
# if defined(ACC_OPENMP_MALLOC)
  result = EXIT_SUCCESS;
  free(item);
# else /* fast allocation */
  if (NULL != item) {
#   pragma omp atomic capture
    i = (*counter)--;
    if (0 <= i && i < maxcount && storage <= item /* check if item came from storage */
      && ((const char*)item) < ((const char*)storage + maxcount * typesize))
    {
      result = EXIT_SUCCESS;
      pointer[i] = item;
    }
    else { /* invalid free */
      result = EXIT_FAILURE;
#     pragma omp atomic
      ++*counter;
    }
  }
  else {
    result = EXIT_SUCCESS;
  }
# endif
#else
  (void)(item); (void)(typesize); (void)(storage); (void)(pointer); (void)(counter); (void)(maxcount); /* unused */
  result = EXIT_FAILURE;
#endif
  assert(0 < acc_openmp_initialized);
  return result;
}


int acc_init(void)
{
  int result;
#if defined(ACC_OPENMP)
# pragma omp atomic
  ++acc_openmp_initialized;
  result = EXIT_SUCCESS;
#else
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_finalize(void)
{
  int result;
#if defined(ACC_OPENMP)
# pragma omp atomic
  --acc_openmp_initialized;
  result = EXIT_SUCCESS;
#else
  result = EXIT_FAILURE;
#endif
  return result;
}


int acc_clear_errors(void)
{
  int result;
#if defined(ACC_OPENMP)
  result = EXIT_SUCCESS;
#else
  result = EXIT_FAILURE;
#endif
  assert(0 < acc_openmp_initialized);
  return result;
}


int acc_get_ndevices(int* n_devices)
{
  int result;
#if !defined(ACC_OPENMP)
  (void)(n_devices); /* unused */
#else
# pragma omp single
  if (NULL != n_devices) {
# if !defined(ACC_OPENMP_DEVICE_MAXCOUNT) || (0 != ACC_OPENMP_DEVICE_MAXCOUNT)
    *n_devices = omp_get_num_devices();
#   if defined(ACC_OPENMP_DEVICE_MAXCOUNT) && (0 < ACC_OPENMP_DEVICE_MAXCOUNT)
    if (ACC_OPENMP_DEVICE_MAXCOUNT < *n_devices) {
      *n_devices = ACC_OPENMP_DEVICE_MAXCOUNT;
    }
#   endif
# else
    *n_devices = 0;
# endif
    result = EXIT_SUCCESS;
  }
  else
#endif
  {
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
