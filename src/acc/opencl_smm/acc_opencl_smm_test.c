/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_opencl_smm.h"


int main(int argc, char* argv[])
{
  int result = EXIT_SUCCESS;
  const int max_kernel_dim = 80;
  int stack_size = 100, offset = 0;
  void *stream = NULL;

  int* dev_trs_stack = NULL;
  double *dev_data = NULL;
  int m = 23, n = 23;

  result = acc_opencl_dbatchtrans(dev_trs_stack, offset, stack_size,
    dev_data, m, n, max_kernel_dim, stream);

  return result;
}

