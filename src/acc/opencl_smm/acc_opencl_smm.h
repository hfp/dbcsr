/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#ifndef ACC_OPENCL_SMM_H
#define ACC_OPENCL_SMM_H

#include "../acc_libsmm.h"
#include "../opencl/acc_opencl.h"

#if defined(__LIBXSMM)
# include <libxsmm.h>
#else
# error OpenCL backend currently depends on LIBXSMM!
#endif

#if !defined(ACC_OPENCL_SMM_KERNELNAME_DEF)
# define ACC_OPENCL_SMM_KERNELNAME_DEF "-DF="
#endif


#if defined(__cplusplus)
extern "C" {
#endif

int acc_opencl_dbatchtrans(const int* dev_trs_stack, int offset, int stack_size,
  double* dev_data, int m, int n, int max_kernel_dim, void* stream);
int acc_opencl_sbatchtrans(const int* dev_trs_stack, int offset, int stack_size,
  float* dev_data, int m, int n, int max_kernel_dim, void* stream);

int acc_opencl_dbatchmm(const libsmm_acc_smmstack_t* host_param_stack,
  const libsmm_acc_smmstack_t* dev_param_stack, int stack_size,
  const double* dev_a_data, const double* dev_b_data, double* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, void* stream);
int acc_opencl_sbatchmm(const libsmm_acc_smmstack_t* host_param_stack,
  const libsmm_acc_smmstack_t* dev_param_stack, int stack_size,
  const float* dev_a_data, const float* dev_b_data, float* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, void* stream);

#if defined(__cplusplus)
}
#endif

#endif /*ACC_OPENCL_SMM_H*/
