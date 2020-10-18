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
  const size_t size = m * n, global_work_size = stack_size * size;
  size_t local_work_size = 128;
  struct { int m, n; } key;
  typedef struct config_t {
    cl_kernel kernel;
    size_t nthreads;
  } config_t;
  config_t* config;
  key.m = m; key.n = n; /* initialize key */
  config = (config_t*)libxsmm_xdispatch(&key, sizeof(key));
  if (NULL == config) {
    FILE *const file = acc_opencl_source_open("transpose.cl", "../../exts/dbcsr/src/acc/opencl_smm/kernel");
    const char *const envnt = getenv("ACC_OPENCL_TRANS_NT");
    const size_t nt = (NULL == envnt ? local_work_size : ((size_t)atoi(envnt)));
    config_t c;
    assert(NULL != acc_opencl_batchtrans_source);
    if (NULL != file) {
      char* lines[50];
      const int nlines = acc_opencl_source(file, lines, sizeof(lines) / sizeof(*lines), 1/*cleanup*/);
      fclose(file);
      result = (0 < nlines ? acc_opencl_kernel((const char**)lines, "build_options", &c.kernel) : EXIT_FAILURE);
    }
    if (EXIT_FAILURE == result
      && sizeof(*acc_opencl_batchtrans_source) <= (sizeof(acc_opencl_batchtrans_source))
      && NULL != *acc_opencl_batchtrans_source)
    {
      result = acc_opencl_kernel(acc_opencl_batchtrans_source, "build_options", &c.kernel);
    }
    else {
      result = EXIT_FAILURE;
    }
    if (EXIT_SUCCESS == result) {
      size_t preferred_multiple;
      result = acc_opencl_wgsize(c.kernel, &preferred_multiple);
      if (EXIT_SUCCESS == result) {
        c.nthreads = LIBXSMM_UP(LIBXSMM_MAX(nt, size), preferred_multiple);
        config = (config_t*)libxsmm_xregister(&key, sizeof(key), sizeof(c), &c);
      }
    }
  }
  if (EXIT_SUCCESS == result) {
    assert(NULL != config);
    ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 0, sizeof(cl_mem), ACC_OPENCL_MEM(dev_trs_stack)),
      "set batch-list argument of transpose kernel", result);
    ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 1, sizeof(int), &offset),
      "set offset argument of transpose kernel", result);
    ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 2, sizeof(cl_mem), ACC_OPENCL_MEM(dev_data)),
      "set matix-data argument of transpose kernel", result);
    ACC_OPENCL_CHECK(clEnqueueNDRangeKernel(*ACC_OPENCL_STREAM(stream), config->kernel, 1/*work_dim*/,
      NULL, &global_work_size, &config->nthreads, 0, NULL, NULL),
      "launch transpose kernel", result);
  }
  return result;
}


int acc_opencl_sbatchtrans(const int* dev_trs_stack, int offset, int stack_size,
  float* dev_data, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_FAILURE; /* TODO */
  return result;
}

#if defined(__cplusplus)
}
#endif
