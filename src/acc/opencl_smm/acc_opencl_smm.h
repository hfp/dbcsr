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

#if defined(__cplusplus)
extern "C" {
#endif

int acc_opencl_template(FILE* source, char* buffer[], int max_nlines, int skip_lines,
  const char* type, const int params[], int nparams);

int acc_opencl_transpose_d(const libsmm_acc_stackdesc_t* dev_trs_stack, int offset, int nblks,
  double* dev_data, int m, int n, int max_kernel_dim, void* stream);
int acc_opencl_transpose_s(const libsmm_acc_stackdesc_t* dev_trs_stack, int offset, int nblks,
  float* dev_data, int m, int n, int max_kernel_dim, void* stream);

int acc_opencl_process_d(const libsmm_acc_stackdesc_t* host_param_stack,
  const libsmm_acc_stackdesc_t* dev_param_stack, int stack_size,
  const double* dev_a_data, const double* dev_b_data, double* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, void* stream);
int acc_opencl_process_s(const libsmm_acc_stackdesc_t* host_param_stack,
  const libsmm_acc_stackdesc_t* dev_param_stack, int stack_size,
  const float* dev_a_data, const float* dev_b_data, float* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, void* stream);

#if defined(__cplusplus)
}
#endif

#endif /*ACC_OPENCL_SMM_H*/
