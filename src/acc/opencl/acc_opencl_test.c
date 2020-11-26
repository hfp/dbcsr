/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#if defined(__OPENCL)
#include "acc_opencl.h"
#include <stdlib.h>

#if !defined(ACC_OPENCL_TEST_MAXNLINES)
# define ACC_OPENCL_TEST_MAXNLINES 50
#endif


int main(int argc, char* argv[])
{
  char* lines[ACC_OPENCL_TEST_MAXNLINES];
  const char *const paths[] = { "smm/kernels" };
  FILE *const file = acc_opencl_source_open(
    1 < argc ? argv[1] : "transpose.cl",
    paths, sizeof(paths) / sizeof(*paths));
  int result = EXIT_SUCCESS, nlines = 0;
  char source[] =
    "/* banner */\n"
    "#define M 23\n"
    "#define N 24\n"
    "/* comment */\n"
    "#define S (M*N)\n"
    "#define T float";
  {
    lines[0] = source;
    nlines = acc_opencl_source(NULL, lines,
      ACC_OPENCL_TEST_MAXNLINES, 1/*cleanup*/);
    if (0 < nlines) {
      if (1 < argc ? 2 == atoi(argv[1]) : 1) {
        int i = 0;
        do {
          ACC_OPENCL_DEBUG_PRINTF("%s\n", lines[i]);
        } while (++i < nlines);
      }
    }
  }
  if (NULL != file) {
    nlines = acc_opencl_source(file, lines,
      ACC_OPENCL_TEST_MAXNLINES, 1/*cleanup*/);
    fclose(file);
    if (0 < nlines) {
      if (1 < argc ? 1 == atoi(argv[1]) : 1) {
        int i = 0;
        do {
          ACC_OPENCL_DEBUG_PRINTF("%s\n", lines[i]);
        } while (++i < nlines);
      }
      free(lines[0]);
    }
  }
  return result;
}

#endif /*__OPENCL*/
