/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "libsmm_omp.h"
#include <assert.h>


#if defined(__cplusplus)
extern "C" {
#endif

int libsmm_acc_transpose_s(const libsmm_acc_stackdesc_t* dev_trs_stack, int offset, int nblks,
  float* dev_data, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_FAILURE; /* TODO */
  return result;
}


int libsmm_acc_transpose_d(const libsmm_acc_stackdesc_t* dev_trs_stack, int offset, int nblks,
  double* dev_data, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_FAILURE; /* TODO */
  return result;
}

#if defined(__cplusplus)
}
#endif
