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


acc_bool_t libsmm_acc_is_thread_safe(void)
{
  return 1/*true*/;
}


int libsmm_acc_transpose(const int* dev_trs_stack, int offset, int stack_size,
  void* dev_data, libsmm_acc_data_t datatype, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_trs_stack && NULL != dev_data && 0 <= stack_size) || 0 == stack_size);
  if (0 < stack_size && 1 < (m * n)) {
    typedef struct config_t {
      cl_kernel kernel;
      size_t wgsize;
    } config_t;
    struct { int m, n; } key;
    config_t *config;
    key.m = m; key.n = n; /* initialize key */
    config = (config_t*)libxsmm_xdispatch(&key, sizeof(key));
    if (NULL == config) {
      const char *const paths[] = {
        "../../exts/dbcsr/src/acc/opencl_smm/kernel"
#if !defined(NDEBUG)
        , "../opencl_smm/kernels"
#endif
      };
      char buffer[ACC_OPENCL_BUFFER_MAXSIZE];
#if 0
      cl_device_id device;
      int level_major, level_minor;
      const char *const level2 = (EXIT_SUCCESS == acc_opencl_device_level(
        (EXIT_SUCCESS == acc_opencl_device(stream, &device) ? device : NULL),
        &level_major, &level_minor) && LIBXSMM_VERSION2(2, 0) <= LIBXSMM_VERSION2(level_major, level_minor))
        ? "-cl-std=CL2.0" : ""; /* OpenCL support level */
#else
      const char *const level2 = "";
#endif
      int nchar = ACC_OPENCL_SNPRINTF(buffer, ACC_OPENCL_BUFFER_MAXSIZE, "xtrans_%i_%i", m, n);
      const char *const fname = ((0 < nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) ? buffer : NULL);
      char *const build_options = (NULL != fname ? (buffer + strlen(fname) + 1) : NULL);
      switch (datatype) {
        case dbcsr_type_real_8: if (NULL != build_options) {
          buffer[0] = 'd';
          nchar = ACC_OPENCL_SNPRINTF(build_options, ACC_OPENCL_BUFFER_MAXSIZE,
            "%s -DT=double -DFN=%s -DSM=%i -DSN=%i", level2, fname, m, n);
        } break;
        case dbcsr_type_real_4: if (NULL != build_options) {
          buffer[0] = 's';
          nchar = ACC_OPENCL_SNPRINTF(build_options, ACC_OPENCL_BUFFER_MAXSIZE,
            "%s -DT=float -DFN=%s -DSM=%i -DSN=%i", level2, fname, m, n);
        } break;
        default: nchar = 0;
      }
      if (0 < nchar && ACC_OPENCL_BUFFER_MAXSIZE > nchar) {
        FILE *const file = acc_opencl_source_open("transpose.cl", paths, sizeof(paths) / sizeof(*paths));
        size_t max_wgsize;
        config_t new_config;
        if (NULL != file) {
          char* lines[50];
          const int nlines = acc_opencl_source(file, lines, sizeof(lines) / sizeof(*lines), 1/*cleanup*/);
          fclose(file);
          result = acc_opencl_kernel((const char**)lines, nlines, build_options, fname, &new_config.kernel);
        }
        assert(NULL != acc_opencl_batchtrans_source);
        if (EXIT_FAILURE == result) {
          if (sizeof(*acc_opencl_batchtrans_source) <= sizeof(acc_opencl_batchtrans_source)
            && NULL != *acc_opencl_batchtrans_source)
          {
            const int nlines = sizeof(acc_opencl_batchtrans_source) / sizeof(*acc_opencl_batchtrans_source);
            result = acc_opencl_kernel(acc_opencl_batchtrans_source, nlines, build_options, fname, &new_config.kernel);
          }
        }
        if (EXIT_SUCCESS == result) {
          result = acc_opencl_wgsize(new_config.kernel, NULL/*preferred_multiple*/, &max_wgsize);
          if (EXIT_SUCCESS == result && max_wgsize < (size_t)n) result = EXIT_FAILURE;
        }
        if (EXIT_SUCCESS == result) {
          const char *const env_wgsize = getenv("ACC_OPENCL_TRANS_WGSIZE");
          if (NULL == env_wgsize) {
            new_config.wgsize = (size_t)n;
          }
          else {
            const int int_wgsize = atoi(env_wgsize);
            new_config.wgsize = LIBXSMM_MIN((size_t)LIBXSMM_MAX(int_wgsize, n), max_wgsize);
          }
          config = (config_t*)libxsmm_xregister(&key, sizeof(key), sizeof(new_config), &new_config);
        }
      }
      else {
        result = EXIT_FAILURE;
      }
    }
    assert((NULL != config && NULL != config->kernel) || EXIT_SUCCESS != result);
    if (EXIT_SUCCESS == result) {
      const size_t work_size = config->wgsize * stack_size;
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 0, sizeof(cl_mem), ACC_OPENCL_MEM(dev_trs_stack)),
        "set batch-list argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 1, sizeof(int), &offset),
        "set offset argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 2, sizeof(cl_mem), ACC_OPENCL_MEM(dev_data)),
        "set matix-data argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clEnqueueNDRangeKernel(*ACC_OPENCL_STREAM(stream),
        config->kernel, 1/*work_dim*/, NULL, &work_size, &config->wgsize, 0, NULL, NULL),
        "launch transpose kernel", result);
    }
  }
  ACC_OPENCL_RETURN(result);
}


int libsmm_acc_process(const int* host_param_stack, const int* dev_param_stack, int stack_size,
  int nparams, libsmm_acc_data_t datatype, const void* dev_a_data, const void* dev_b_data, void* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, acc_bool_t def_mnk, void* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_param_stack && NULL != dev_a_data && NULL != dev_b_data && NULL != dev_c_data) || 0 == stack_size);
  assert((nparams * sizeof(int)) == sizeof(libsmm_acc_smmstack_t));
  if (0 != stack_size && def_mnk/*homogeneous*/) {
    libxsmm_init();
    switch (datatype) {
      case dbcsr_type_real_8: {
        result = acc_opencl_dbatchmm(
          (const libsmm_acc_smmstack_t*)host_param_stack,
          (const libsmm_acc_smmstack_t*)dev_param_stack, stack_size,
          (const double*)dev_a_data, (const double*)dev_b_data, (double*)dev_c_data,
          m_max, n_max, k_max, max_kernel_dim, stream);
      } break;
      case dbcsr_type_real_4: {
        result = acc_opencl_sbatchmm(
          (const libsmm_acc_smmstack_t*)host_param_stack,
          (const libsmm_acc_smmstack_t*)dev_param_stack, stack_size,
          (const float*)dev_a_data, (const float*)dev_b_data, (float*)dev_c_data,
          m_max, n_max, k_max, max_kernel_dim, stream);
      } break;
      default: {
        result = EXIT_FAILURE;
      }
    }
  }
  else if (0 != stack_size) { /* inhomogeneous */
    /* stream status: do not flag an error */
    result = EXIT_FAILURE; /* reject work */
  }
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif
