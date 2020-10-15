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


int main(int argc, char* argv[])
{
  char* buffer[ACC_OPENCL_TEST_MAXNLINES];
  FILE *const file = fopen("../opencl_smm/kernels/acc_opencl_smm_transpose.cl", "r");
  int result = EXIT_SUCCESS, nlines = 0;

  if (NULL != file) {
    nlines = acc_opencl_source(file, buffer,
      ACC_OPENCL_TEST_MAXNLINES, 1/*cleanup*/);
    if (0 < nlines) {
      if (1 < argc ? 1 == atoi(argv[1]) : 1) {
        int i = 0;
        do {
          ACC_OPENCL_DEBUG_PRINTF("%s\n", buffer[i]);
        } while (++i < nlines);
      }
      free(buffer[0]);
    }
    fclose(file);
  }

  { char source[] =
      "/* banner */\n"
      "#define M 23\n"
      "#define N 24\n"
      "/* comment */\n"
      "#define S (M*N)\n"
      "#define T float";
    buffer[0] = source;
    nlines = acc_opencl_source(NULL, buffer,
      ACC_OPENCL_TEST_MAXNLINES, 1/*cleanup*/);
    if (0 < nlines) {
      if (1 < argc ? 2 == atoi(argv[1]) : 1) {
        int i = 0;
        do {
          ACC_OPENCL_DEBUG_PRINTF("%s\n", buffer[i]);
        } while (++i < nlines);
      }
    }
  }

  return result;
}

