/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_opencl_smm.h"
#include <assert.h>


#if defined(__cplusplus)
extern "C" {
#endif

int acc_opencl_dbatchtrans(const libsmm_acc_stackdesc_t* dev_trs_stack, int offset, int stack_size,
  double* dev_data, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_FAILURE; /* TODO */
  const char *const envs = getenv("ACC_OPENCL_TRANS_S");
  const int s = (NULL == envs ? 0/*TODO*/ : atoi(envs));
  struct { int m, n; } key;
  key.m = m;
  key.n = n;
  cl_kernel kernel = (cl_kernel)libxsmm_xdispatch(&key, sizeof(key));
  if (NULL == kernel) {
    /* TODO: create kernel here; use ACC_OPENCL_MAX(m*n, s) */
    libxsmm_xregister(&key, sizeof(key), sizeof(kernel), kernel);
  }
  clSetKernelArg(kernel, 0, sizeof(cl_mem), ACC_OPENCL_MEM(dev_trs_stack));
  assert(0 == offset % sizeof(libsmm_acc_stackdesc_t));
  clSetKernelArg(kernel, 1, sizeof(int), &offset);
  clSetKernelArg(kernel, 2, sizeof(cl_mem), ACC_OPENCL_MEM(dev_data));
  return result;
}


int acc_opencl_sbatchtrans(const libsmm_acc_stackdesc_t* dev_trs_stack, int offset, int stack_size,
  float* dev_data, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_FAILURE; /* TODO */
  return result;
}

#if defined(__cplusplus)
}
#endif
