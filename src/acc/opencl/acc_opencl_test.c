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
  const int params[] = { 23, 24, 24*24 };
  FILE *const source = fopen("../opencl_smm/kernels/acc_opencl_smm_transpose.cl", "r");
  int result = EXIT_SUCCESS, nlines = 0;

  if (NULL != source) {
    nlines = acc_opencl_template(source, buffer, ACC_OPENCL_TEST_MAXNLINES,
      8/*skip banner*/, "double", params, sizeof(params) / sizeof(*params));
    if (0 < nlines && (1 < argc ? 1 == atoi(argv[1]) : 1)) {
      int i = 0;
      do {
        ACC_OPENCL_DEBUG_PRINTF("%s", buffer[i]);
      } while (++i < nlines);
    }
    fclose(source);
  }
  if (0 < nlines) {
    if (NULL == buffer[nlines]) {
      if (nlines == acc_opencl_template(NULL, buffer, ACC_OPENCL_TEST_MAXNLINES,
        0, "double", params, sizeof(params) / sizeof(*params)))
      {
        if (0 < nlines && (1 < argc ? 2 == atoi(argv[1]) : 1)) {
          int i = 0;
          do {
            ACC_OPENCL_DEBUG_PRINTF("%s", buffer[i]);
          } while (++i < nlines);
        }
      }
      else {
        result = EXIT_FAILURE;
      }
    }
    else {
      result = EXIT_FAILURE;
    }
    free(buffer[0]);
  }
  else if (NULL != buffer[0]) {
    result = EXIT_FAILURE;
  }
#if 0
  if (EXIT_SUCCESS == result || NULL == source) {
    char test[] =
      "/* header */\n"
      "#define M %i\n"
      "#define N %i\n"
      "/* comment */\n"
      "#define S %i\n"
      "#define T %s";
    buffer[0] = test;
    nlines = acc_opencl_template(NULL, buffer,
      6, 1/*header*/, "float", params,
      sizeof(params) / sizeof(*params));
    if (0 < nlines) {
      if (1 < argc ? 3 == atoi(argv[1]) : 1) {
        int i = 0;
        do {
          ACC_OPENCL_DEBUG_PRINTF("%s", buffer[i]);
        } while (++i < nlines);
      }
      free(buffer[0]);
    }
    result = EXIT_SUCCESS;
  }
#endif
  return result;
}

