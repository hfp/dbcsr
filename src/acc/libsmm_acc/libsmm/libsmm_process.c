/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "libsmm.h"
#include <stdlib.h>
#include <assert.h>


#if defined(__cplusplus)
extern "C" {
#endif

#if defined(ACC_OPENMP_OFFLOAD)
# pragma omp declare target
#endif
int libsmm_acc_process_d(const libsmm_acc_stack_descriptor_type* dev_param_stack, int stack_size,
  int nparams, const double* dev_a_data, const double* dev_b_data, double* dev_c_data,
  int m_max, int n_max, int k_max)
{
  int result = EXIT_FAILURE; /* TODO */
  return result;
}
#if defined(ACC_OPENMP_OFFLOAD)
# pragma omp end declare target
#endif


#if defined(ACC_OPENMP_OFFLOAD)
# pragma omp declare target
#endif
int libsmm_acc_process_s(const libsmm_acc_stack_descriptor_type* dev_param_stack, int stack_size,
  int nparams, const float* dev_a_data, const float* dev_b_data, float* dev_c_data,
  int m_max, int n_max, int k_max)
{
  int result = EXIT_FAILURE; /* TODO */
  return result;
}
#if defined(ACC_OPENMP_OFFLOAD)
# pragma omp end declare target
#endif

#if defined(__cplusplus)
}
#endif
