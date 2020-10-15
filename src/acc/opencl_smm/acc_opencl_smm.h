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

#define ACC_OPENCL_CONCAT2(A, B) A##B
#define ACC_OPENCL_CONCAT3(A, B, C) A##B##C
#define ACC_OPENCL_CONCAT4(A, B, C, D) A##B##C##D
#define ACC_OPENCL_CONCAT5(A, B, C, D, E) A##B##C##D##E
#define ACC_OPENCL_CONCAT6(A, B, C, D, E, F) A##B##C##D##E##F
#define ACC_OPENCL_CONCAT7(A, B, C, D, E, F, G) A##B##C##D##E##F##G
#define ACC_OPENCL_CONCAT8(A, B, C, D, E, F, G, H) A##B##C##D##E##F##G##H

#define ACC_OPENCL_TYPE_CHAR(T) ACC_OPENCL_CONCAT2(ACC_OPENCL_TYPE_CHAR_, T)
#define ACC_OPENCL_TYPE_CHAR_double d
#define ACC_OPENCL_TYPE_CHAR_float s

#define ACC_OPENCL_FUNCTION2(FN, T, A, B) ACC_OPENCL_CONCAT6(ACC_OPENCL_TYPE_CHAR(T), FN, _, A, _, B)
#define ACC_OPENCL_FUNCTION3(FN, T, A, B, C) ACC_OPENCL_CONCAT7(ACC_OPENCL_TYPE_CHAR(T), FN, _, A, _, B, _, C)


#if defined(__cplusplus)
extern "C" {
#endif

int acc_opencl_dbatchtrans(const libsmm_acc_stackdesc_t* dev_trs_stack, int offset, int nblks,
  double* dev_data, int m, int n, int max_kernel_dim, void* stream);
int acc_opencl_sbatchtrans(const libsmm_acc_stackdesc_t* dev_trs_stack, int offset, int nblks,
  float* dev_data, int m, int n, int max_kernel_dim, void* stream);

int acc_opencl_dbatchmm(const libsmm_acc_stackdesc_t* host_param_stack,
  const libsmm_acc_stackdesc_t* dev_param_stack, int stack_size,
  const double* dev_a_data, const double* dev_b_data, double* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, void* stream);
int acc_opencl_sbatchmm(const libsmm_acc_stackdesc_t* host_param_stack,
  const libsmm_acc_stackdesc_t* dev_param_stack, int stack_size,
  const float* dev_a_data, const float* dev_b_data, float* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, void* stream);

#if defined(__cplusplus)
}
#endif

#endif /*ACC_OPENCL_SMM_H*/
