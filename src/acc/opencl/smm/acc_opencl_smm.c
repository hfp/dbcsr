/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#if defined(__OPENCL)
#include "acc_opencl_smm.h"
#include <assert.h>


#if defined(__cplusplus)
extern "C" {
#endif

const char* acc_opencl_batchtrans_source[] = {
  NULL
};


const char* acc_opencl_batchmm_source[] = {
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
    /*LIBXSMM_MEMZERO127(&key);*/
    key.m = m; key.n = n; /* initialize key */
    config = (config_t*)libxsmm_xdispatch(&key, sizeof(key));
    if (NULL == config) {
      char build_options[512], fname[16];
      const char *const env_options = getenv("ACC_OPENCL_TRANS_BUILD_OPTIONS");
#if defined(ACC_OPENCL_SMM_PERMIT_TRANSPOSE_TINY) && (0 < ACC_OPENCL_SMM_PERMIT_TRANSPOSE_TINY)
      const int local = (ACC_OPENCL_SMM_PERMIT_TRANSPOSE_TINY >= m ? 0/*private*/ : 1/*local*/);
#else
      const char *const env_tiny = getenv("ACC_OPENCL_TRANS_TINY");
      const int tiny = ((NULL == env_tiny || '0' == *env_tiny) ? 0 : atoi(env_tiny));
      const int local = ((0 == tiny || (1 < tiny && tiny < m)) ? 1/*local*/ : 0/*private*/);
#endif
      int nchar = ACC_OPENCL_SNPRINTF(fname, sizeof(fname), "xtrans%ix%i", m, n);
      const char* typename = "";
      switch (datatype) {
        case dbcsr_type_real_8: {
          typename = "char8"; /* double */
          fname[0] = 'd';
        } break;
        case dbcsr_type_real_4: {
          typename = "float";
          fname[0] = 's';
        } break;
        default: ;
      }
      nchar = ((0 < nchar && (int)sizeof(fname) > nchar)
        ? ACC_OPENCL_SNPRINTF(build_options, sizeof(build_options), "%s -DKIND=%s -DT=%s -DFN=%s -DSM=%i -DSN=%i",
        (NULL == env_options || '\0' == *env_options) ? "" : env_options,
        local ? "local" : "private", typename, fname, m, n) : 0);
      if ('\0' != *typename && 0 < nchar && (int)sizeof(build_options) > nchar) {
#if !defined(ACC_OPENCL_SMM_PERMIT_TRANSPOSE_INPLACE)
        const char *const env_inplace = getenv("ACC_OPENCL_TRANS_INPLACE");
#endif
        const char *const paths[] = {
          "../../exts/dbcsr/src/acc/opencl/smm/kernel",
          "opencl/smm/kernels"
        };
        FILE *const file = acc_opencl_source_open(
#if defined(ACC_OPENCL_SMM_PERMIT_TRANSPOSE_INPLACE)
          (m == n) ? "transpose_inplace.cl" :
#else
          (m == n && (NULL == env_inplace || '0' != *env_inplace)) ? "transpose_inplace.cl" :
#endif
          "transpose.cl",
          paths, sizeof(paths) / sizeof(*paths));
        int max_wgsize;
        config_t new_config;
        if (NULL != file) {
          char* lines[50];
          const int nlines = acc_opencl_source(file, lines, sizeof(lines) / sizeof(*lines),
            /* whether to cleanup the loaded source code or not */
#if defined(NDEBUG)
            1);
#else
            0);
#endif
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
        }
        if (EXIT_SUCCESS == result) {
          const char *const env_wgsize = getenv("ACC_OPENCL_TRANS_WGSIZE");
          if (NULL == env_wgsize || '\0' == *env_wgsize) {
            new_config.wgsize = (size_t)m;
          }
          else {
            const int int_wgsize = atoi(env_wgsize);
            if (0 < int_wgsize) {
              new_config.wgsize = (size_t)((m <= int_wgsize || 0 == (m % int_wgsize)) ? int_wgsize : m);
            }
            else {
              new_config.wgsize = 0;
            }
          }
          if (max_wgsize < new_config.wgsize || 0 == local) new_config.wgsize = 1;
          config = (config_t*)libxsmm_xregister(&key, sizeof(key), sizeof(new_config), &new_config);
        }
      }
      else {
        result = EXIT_FAILURE;
      }
    }
    assert((NULL != config && NULL != config->kernel) || EXIT_SUCCESS != result);
    if (EXIT_SUCCESS == result) {
      const size_t work_size = (0 < config->wgsize ? config->wgsize : m) * stack_size;
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 0, sizeof(cl_mem), ACC_OPENCL_MEM(dev_trs_stack)),
        "set batch-list argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 1, sizeof(int), &offset),
        "set offset argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 2, sizeof(cl_mem), ACC_OPENCL_MEM(dev_data)),
        "set matix-data argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clEnqueueNDRangeKernel(*ACC_OPENCL_STREAM(stream),
        config->kernel, 1/*work_dim*/, NULL, &work_size, 0 < config->wgsize ? &config->wgsize : NULL, 0, NULL, NULL),
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
#if 0
  assert((NULL != dev_param_stack && NULL != dev_a_data && NULL != dev_b_data && NULL != dev_c_data) || 0 == stack_size);
  assert((nparams * sizeof(int)) == sizeof(libsmm_acc_smmstack_t));
  if (0 != stack_size && def_mnk/*homogeneous*/) {
  }
  else if (0 != stack_size) { /* inhomogeneous */
    /* stream status: do not flag an error */
    result = EXIT_FAILURE; /* reject work */
  }
#endif
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif

#endif /*__OPENCL*/
