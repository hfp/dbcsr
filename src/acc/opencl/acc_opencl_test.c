/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "acc_opencl.h"

#if !defined(ACC_OPENCL_TEST_MAXNLINES)
# define ACC_OPENCL_TEST_MAXNLINES 50
#endif


int main(void)
{
  char* buffer[ACC_OPENCL_TEST_MAXNLINES];
  const int params[] = { 23, 24, 24*24 };
  FILE *const source = fopen("../opencl_smm/kernels/acc_opencl_smm_transpose.cl", "r");
  if (NULL != source) {
    const int nlines = acc_opencl_template(source, buffer, ACC_OPENCL_TEST_MAXNLINES,
      8/*skip banner*/, "double", params, sizeof(params) / sizeof(*params));
    if (0 < nlines) {
      int i = 0;
      do {
        printf("%s", buffer[i]);
        ++i;
      } while (i < nlines);
      free(buffer[0]);
    }
    fclose(source);
  }
  return EXIT_SUCCESS;
}

