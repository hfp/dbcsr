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
  const size_t global_work_size = stack_size * n;
  struct { int m, n; } key;
  typedef struct config_t {
    cl_kernel kernel;
    size_t wgsize;
  } config_t;
  config_t* config;
  key.m = m; key.n = n; /* initialize key */
  config = (config_t*)libxsmm_xdispatch(&key, sizeof(key));
  if (NULL == config) {
    config_t c;
    const char *const paths[] = {
      "../../exts/dbcsr/src/acc/opencl_smm/kernel"
#if !defined(NDEBUG)
      , "../opencl_smm/kernels"
#endif
    };
    char buffer[ACC_OPENCL_BUFFER_MAXSIZE];
    const int fsize = ACC_OPENCL_SNPRINTF(buffer, ACC_OPENCL_BUFFER_MAXSIZE, "dtrans_%i_%i", m, n);
    char *const build_options = ((0 < fsize && ACC_OPENCL_BUFFER_MAXSIZE > fsize) ? (buffer + strlen(buffer) + 1) : NULL);
    cl_device_id device;
    int level_major, level_minor;
    const char *const level2 = (EXIT_SUCCESS == acc_opencl_device_level(
      (EXIT_SUCCESS == acc_opencl_device(stream, &device) ? device : NULL),
      &level_major, &level_minor) && LIBXSMM_VERSION2(2, 0) <= LIBXSMM_VERSION2(level_major, level_minor))
      ? "-cl-std=CL2.0" : ""; /* OpenCL support level */
    const int nchar = (NULL != build_options ? ACC_OPENCL_SNPRINTF(build_options, ACC_OPENCL_BUFFER_MAXSIZE,
      "%s -DT=double -DFN=%s -DSM=%i -DSN=%i", level2, buffer, m, n) : 0);
    if (0 < nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) {
      FILE *const file = acc_opencl_source_open("transpose.cl", paths, sizeof(paths) / sizeof(*paths));
      if (NULL != file) {
        char* lines[50];
        const int nlines = acc_opencl_source(file, lines, sizeof(lines) / sizeof(*lines), 1/*cleanup*/);
        fclose(file);
        result = acc_opencl_kernel((const char**)lines, nlines, build_options, buffer, &c.kernel);
      }
      assert(NULL != acc_opencl_batchtrans_source);
      if (EXIT_FAILURE == result) {
        if (sizeof(*acc_opencl_batchtrans_source) <= sizeof(acc_opencl_batchtrans_source)
          && NULL != *acc_opencl_batchtrans_source)
        {
          const int nlines = sizeof(acc_opencl_batchtrans_source) / sizeof(*acc_opencl_batchtrans_source);
          result = acc_opencl_kernel(acc_opencl_batchtrans_source, nlines, build_options, buffer, &c.kernel);
        }
      }
    }
    else {
      result = EXIT_FAILURE;
    }
    if (EXIT_SUCCESS == result) {
      if ('\0' != *level2) { /* support-level at least OpenCL 2.0 */
        size_t preferred_multiple, max_wgsize;
        result = acc_opencl_wgsize(c.kernel, &preferred_multiple, &max_wgsize);
        if (EXIT_SUCCESS == result) {
          const char *const env_wgsize = getenv("ACC_OPENCL_TRANS_WGSIZE");
          const int int_wgsize = (NULL == env_wgsize ? atoi(env_wgsize) : n);
          const size_t wgsize = (size_t)(0 <= int_wgsize ? int_wgsize : n);
          c.wgsize = LIBXSMM_MIN(LIBXSMM_UP(wgsize, preferred_multiple), max_wgsize);
          config = (config_t*)libxsmm_xregister(&key, sizeof(key), sizeof(c), &c);
        }
      }
      else {
        c.wgsize = 0;
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
      NULL, &global_work_size, 0 != config->wgsize ? &config->wgsize : NULL, 0, NULL, NULL),
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
