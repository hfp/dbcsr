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

const char* acc_opencl_batchtrans_source[] = {
  NULL
};


int acc_opencl_dbatchtrans(const int* dev_trs_stack, int offset, int stack_size,
  double* dev_data, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_SUCCESS;
  struct { int m, n; } key;
  cl_kernel *kernel;
  key.m = m; key.n = n; /* initialize key */
  kernel = (cl_kernel*)libxsmm_xdispatch(&key, sizeof(key));
  if (NULL == kernel) {
    const char *const paths[] = {
      "../../exts/dbcsr/src/acc/opencl_smm/kernel"
#if !defined(NDEBUG)
      , "../opencl_smm/kernels"
#endif
    };
    char buffer[ACC_OPENCL_BUFFER_MAXSIZE];
    const int fsize = ACC_OPENCL_SNPRINTF(buffer, ACC_OPENCL_BUFFER_MAXSIZE, "dtrans_%i_%i", m, n);
    char *const build_options = ((0 < fsize && ACC_OPENCL_BUFFER_MAXSIZE > fsize) ? (buffer + strlen(buffer) + 1) : NULL);
    const int nchar = (NULL != build_options ? ACC_OPENCL_SNPRINTF(build_options, ACC_OPENCL_BUFFER_MAXSIZE,
      "-DT=double -DFN=%s -DSM=%i -DSN=%i", buffer, m, n) : 0);
    if (0 < nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) {
      FILE *const file = acc_opencl_source_open("transpose.cl", paths, sizeof(paths) / sizeof(*paths));
      cl_kernel new_kernel;
      if (NULL != file) {
        char* lines[50];
        const int nlines = acc_opencl_source(file, lines, sizeof(lines) / sizeof(*lines), 1/*cleanup*/);
        fclose(file);
        result = acc_opencl_kernel((const char**)lines, nlines, build_options, buffer, &new_kernel);
      }
      assert(NULL != acc_opencl_batchtrans_source);
      if (EXIT_FAILURE == result) {
        if (sizeof(*acc_opencl_batchtrans_source) <= sizeof(acc_opencl_batchtrans_source)
          && NULL != *acc_opencl_batchtrans_source)
        {
          const int nlines = sizeof(acc_opencl_batchtrans_source) / sizeof(*acc_opencl_batchtrans_source);
          result = acc_opencl_kernel(acc_opencl_batchtrans_source, nlines, build_options, buffer, &new_kernel);
        }
      }
      if (EXIT_SUCCESS == result) {
        kernel = (cl_kernel*)libxsmm_xregister(&key, sizeof(key), sizeof(cl_kernel), &new_kernel);
      }
    }
    else {
      result = EXIT_FAILURE;
    }
  }
  if (EXIT_SUCCESS == result) {
    const size_t wgsize = n, work_size = wgsize * stack_size;
    assert(NULL != kernel && NULL != *kernel);
    ACC_OPENCL_CHECK(clSetKernelArg(*kernel, 0, sizeof(cl_mem), ACC_OPENCL_MEM(dev_trs_stack)),
      "set batch-list argument of transpose kernel", result);
    ACC_OPENCL_CHECK(clSetKernelArg(*kernel, 1, sizeof(int), &offset),
      "set offset argument of transpose kernel", result);
    ACC_OPENCL_CHECK(clSetKernelArg(*kernel, 2, sizeof(cl_mem), ACC_OPENCL_MEM(dev_data)),
      "set matix-data argument of transpose kernel", result);
    ACC_OPENCL_CHECK(clEnqueueNDRangeKernel(*ACC_OPENCL_STREAM(stream),
      *kernel, 1/*work_dim*/, NULL, &work_size, &wgsize, 0, NULL, NULL),
      "launch transpose kernel", result);
  }
  ACC_OPENCL_RETURN(result);
}


int acc_opencl_sbatchtrans(const int* dev_trs_stack, int offset, int stack_size,
  float* dev_data, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_FAILURE; /* TODO */
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
