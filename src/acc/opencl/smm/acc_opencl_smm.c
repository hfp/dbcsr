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

int* acc_opencl_smm_locks, acc_opencl_smm_nlocks;


const char* acc_opencl_batchtrans_source[] = {
  NULL
};


const char* acc_opencl_batchmm_source[] = {
  NULL
};


int libsmm_acc_init(void)
{
  const int result = (0 == acc_opencl_smm_nlocks
    ? acc_init() : EXIT_FAILURE);
  ACC_OPENCL_RETURN(result);
}


static int libsmm_acc_finalize_locks(void);
static int libsmm_acc_finalize_locks(void)
{
  int *const smm_locks = acc_opencl_smm_locks;
  acc_opencl_smm_nlocks = 0;
  acc_opencl_smm_locks = NULL;
  return acc_dev_mem_deallocate(smm_locks);
}


int libsmm_acc_finalize(void)
{
  ACC_OPENCL_RETURN(libsmm_acc_finalize_locks());
}


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
    /* homogeneous key-data (no need for prior memset) */
    key.m = m; key.n = n; /* initialize key */
    config = (config_t*)libxsmm_xdispatch(&key, sizeof(key));
    if (NULL == config) {
      char build_options[ACC_OPENCL_BUFFER_MAXSIZE], fname[32];
      const char *const env_options = getenv("ACC_OPENCL_TRANS_BUILD_OPTIONS");
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
        ? ACC_OPENCL_SNPRINTF(build_options, sizeof(build_options), "%s -DT=%s -DFN=%s -DSM=%i -DSN=%i",
        (NULL == env_options || '\0' == *env_options) ? "" : env_options, typename, fname, m, n) : 0);
      if ('\0' != *typename && 0 < nchar && (int)sizeof(build_options) > nchar) {
        const char *const env_inplace = getenv("ACC_OPENCL_TRANS_INPLACE");
        cl_device_id active_device;
        const int inplace = (m == n &&
          ((NULL != env_inplace && '\0' != *env_inplace && '0' != *env_inplace)
            ||  (EXIT_SUCCESS == acc_opencl_device(stream, &active_device)
              && EXIT_SUCCESS == acc_opencl_device_vendor(active_device, "intel"))));
        const char *const paths[] = {
          "../../exts/dbcsr/src/acc/opencl/smm/kernel",
          "opencl/smm/kernels"
        };
        FILE *const file = acc_opencl_source_open(
          inplace ? "transpose_inplace.cl" : "transpose.cl",
          paths, sizeof(paths) / sizeof(*paths));
        config_t new_config;
        if (NULL != file) {
          char* lines[128];
          const int nlines = acc_opencl_source(file, lines,
            NULL/*extensions*/, sizeof(lines) / sizeof(*lines),
            /* whether to cleanup the loaded source code or not */
#if defined(NDEBUG)
            1);
#else
            0);
#endif
          fclose(file);
          result = acc_opencl_kernel((const char**)lines, nlines, build_options, fname, &new_config.kernel);
          if (0 < nlines) free(lines[0]);
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
          int max_wgsize;
          result = acc_opencl_wgsize(new_config.kernel, NULL/*preferred_multiple*/, &max_wgsize);
          if (EXIT_SUCCESS == result) {
            const char *const env_wgsize = getenv("ACC_OPENCL_TRANS_WGSIZE");
            assert(0 < max_wgsize);
            if (NULL == env_wgsize || '\0' == *env_wgsize) {
              new_config.wgsize = (size_t)m;
            }
            else {
              const int int_wgsize = atoi(env_wgsize);
              new_config.wgsize = (size_t)((m <= int_wgsize || 0 == (m % int_wgsize)) ? int_wgsize : m);
            }
            if (max_wgsize < (int)new_config.wgsize) new_config.wgsize = 1;
            config = (config_t*)libxsmm_xregister(&key, sizeof(key), sizeof(new_config), &new_config);
          }
        }
      }
      else {
        result = EXIT_FAILURE;
      }
    }
    assert((NULL != config && NULL != config->kernel && 0 < config->wgsize) || EXIT_SUCCESS != result);
    if (EXIT_SUCCESS == result) {
      const size_t work_size = config->wgsize * stack_size;
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 0, sizeof(cl_mem), ACC_OPENCL_MEM(dev_trs_stack)),
        "set batch-list argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 1, sizeof(int), &offset),
        "set offset argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 2, sizeof(cl_mem), ACC_OPENCL_MEM(dev_data)),
        "set matrix-data argument of transpose kernel", result);
      ACC_OPENCL_CHECK(clEnqueueNDRangeKernel(*ACC_OPENCL_STREAM(stream),
        config->kernel, 1/*work_dim*/, NULL, &work_size, &config->wgsize, 0, NULL, NULL),
        "launch transpose kernel", result);
    }
  }
  ACC_OPENCL_RETURN(result);
}


int libsmm_acc_process(const int* host_param_stack, const int* dev_param_stack, int stack_size,
  int nparams, libsmm_acc_data_t datatype, const void* dev_a_data, const void* dev_b_data, void* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, acc_bool_t def_mnk, void* stack_stream, void* c_stream)
{
  int result = EXIT_SUCCESS;
  ACC_OPENCL_UNUSED(c_stream);
  assert((NULL != dev_param_stack && NULL != dev_a_data && NULL != dev_b_data && NULL != dev_c_data) || 0 == stack_size);
  assert(0 < nparams && 0 < max_kernel_dim && NULL != stack_stream);
  if (0 <= stack_size) {
    if (0 < stack_size && def_mnk/*homogeneous*/ &&
        0 < m_max && m_max <= max_kernel_dim &&
        0 < n_max && n_max <= max_kernel_dim &&
        0 < k_max && k_max <= max_kernel_dim)
    {
      typedef struct config_t {
        cl_kernel kernel;
        size_t wgsize;
      } config_t;
      struct { int m, n, k; } key;
      config_t *config;
      /* homogeneous key-data (no need for prior memset) */
      key.m = m_max; key.n = n_max; key.k = k_max; /* initialize key */
      config = (config_t*)libxsmm_xdispatch(&key, sizeof(key));
      if (NULL == config) {
        char build_options[ACC_OPENCL_BUFFER_MAXSIZE], fname[48];
        int nchar = ACC_OPENCL_SNPRINTF(fname, sizeof(fname), "xmm%ix%ix%i", m_max, n_max, k_max);
        const char* extensions = NULL;
        if (0 < nchar && (int)sizeof(fname) > nchar) {
          cl_device_id active_device;
          result = acc_opencl_device(stack_stream, &active_device);
          if (0 == acc_opencl_smm_nlocks && EXIT_SUCCESS == result) {
            const char *const env_nlocks = getenv("ACC_OPENCL_SMM_NLOCKS");
            const int nlocks = ((NULL == env_nlocks || '\0' == *env_nlocks) ? 16 : atoi(env_nlocks));
            acc_opencl_smm_nlocks = LIBXSMM_UP2POT(nlocks);
            result = acc_dev_mem_allocate((void**)&acc_opencl_smm_locks,
              sizeof(int) * (acc_opencl_smm_nlocks * 2));
            if (EXIT_SUCCESS == result) {
              result = acc_memset_zero(acc_opencl_smm_locks, 0/*offset*/,
                sizeof(int) * (acc_opencl_smm_nlocks * 2), stack_stream);
            }
            else {
              ACC_OPENCL_EXPECT(EXIT_SUCCESS, libsmm_acc_finalize_locks());
              acc_opencl_smm_nlocks = 0;
            }
          }
          if (EXIT_SUCCESS == result) {
            const char *const env_options = getenv("ACC_OPENCL_SMM_BUILD_OPTIONS");
            const char *typename = NULL, *atomic_t = NULL, *atomic_f = NULL;
            assert(NULL != active_device);
            switch (datatype) {
              case dbcsr_type_real_8: {
                extensions = "cl_khr_global_int32_base_atomics cl_khr_fp64 cl_khr_int64_base_atomics";
                if (EXIT_SUCCESS == acc_opencl_device_ext(active_device, &extensions, 1)) {
                  typename = "double";
                  atomic_t = "long";
                  atomic_f = "atom_cmpxchg";
                  fname[0] = 'd';
                }
              } break;
              case dbcsr_type_real_4: {
                extensions = "cl_khr_global_int32_base_atomics";
                if (EXIT_SUCCESS == acc_opencl_device_ext(active_device, &extensions, 1)) {
                  typename = "float";
                  atomic_t = "int";
                  atomic_f = "atomic_cmpxchg";
                  fname[0] = 's';
                }
              } break;
              default: ;
            }
            if (NULL != typename && '\0' != *typename) {
              const char *const build_setup = "%s -cl-fast-relaxed-math"
                " -DT=%s -DTA=\"%s\" -DFA=%s -DFN=%s -DNLOCKS=%i -DSM=%i -DSN=%i -DSK=%i";
              nchar = ACC_OPENCL_SNPRINTF(build_options, sizeof(build_options), build_setup,
                (NULL == env_options || '\0' == *env_options) ? "" : env_options,
                typename, atomic_t, atomic_f, fname, acc_opencl_smm_nlocks, m_max, n_max, k_max);
              if (0 >= nchar || (int)sizeof(build_options) <= nchar) result = EXIT_FAILURE;
            }
            else {
              result = EXIT_FAILURE;
              ACC_OPENCL_ERROR("insufficient device capabilities", result);
            }
          }
        }
        else {
          result = EXIT_FAILURE;
        }
        if (EXIT_SUCCESS == result) {
          const char *const paths[] = {
            "../../exts/dbcsr/src/acc/opencl/smm/kernel",
            "opencl/smm/kernels"
          };
          FILE *const file = acc_opencl_source_open("multiply.cl", paths, sizeof(paths) / sizeof(*paths));
          config_t new_config;
          if (NULL != file) {
            char* lines[128];
            const int nlines = acc_opencl_source(file, lines,
              extensions, sizeof(lines) / sizeof(*lines),
              /* whether to cleanup the loaded source code or not */
#if defined(NDEBUG)
              1);
#else
              0);
#endif
            fclose(file);
            result = acc_opencl_kernel((const char**)lines, nlines, build_options, fname, &new_config.kernel);
            if (0 < nlines) free(lines[0]);
          }
          assert(NULL != acc_opencl_batchmm_source);
          if (EXIT_FAILURE == result) {
            if (sizeof(*acc_opencl_batchmm_source) <= sizeof(acc_opencl_batchmm_source)
              && NULL != *acc_opencl_batchmm_source)
            {
              const int nlines = sizeof(acc_opencl_batchmm_source) / sizeof(*acc_opencl_batchmm_source);
              result = acc_opencl_kernel(acc_opencl_batchmm_source, nlines, build_options, fname, &new_config.kernel);
            }
          }
          if (EXIT_SUCCESS == result) {
            int max_wgsize;
            result = acc_opencl_wgsize(new_config.kernel, NULL/*preferred_multiple*/, &max_wgsize);
            if (EXIT_SUCCESS == result) {
              const char *const env_wgsize = getenv("ACC_OPENCL_SMM_WGSIZE");
              assert(0 < max_wgsize);
              if (NULL == env_wgsize || '\0' == *env_wgsize) {
                new_config.wgsize = (size_t)n_max;
              }
              else {
                const int int_wgsize = atoi(env_wgsize);
                new_config.wgsize = (size_t)((n_max <= int_wgsize || 0 == (n_max % int_wgsize)) ? int_wgsize : n_max);
              }
              if (max_wgsize < (int)new_config.wgsize) new_config.wgsize = 1;
              config = (config_t*)libxsmm_xregister(&key, sizeof(key), sizeof(new_config), &new_config);
            }
          }
        }
        else {
          result = EXIT_FAILURE;
        }
      }
      assert((NULL != config && NULL != config->kernel && 0 < config->wgsize) || EXIT_SUCCESS != result);
      assert((NULL != acc_opencl_smm_locks && 0 != acc_opencl_smm_nlocks) || EXIT_SUCCESS != result);
      if (EXIT_SUCCESS == result) {
        const size_t work_size = config->wgsize * stack_size;
        ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 0, sizeof(cl_mem), ACC_OPENCL_MEM(dev_param_stack)),
          "set batch-list argument of SMM-kernel", result);
        ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 1, sizeof(cl_mem), ACC_OPENCL_MEM(acc_opencl_smm_locks)),
          "set lock argument of SMM-kernel", result);
        ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 2, sizeof(cl_mem), ACC_OPENCL_MEM(dev_a_data)),
          "set A-matrix argument of SMM-kernel", result);
        ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 3, sizeof(cl_mem), ACC_OPENCL_MEM(dev_b_data)),
          "set B-matrix argument of SMM-kernel", result);
        ACC_OPENCL_CHECK(clSetKernelArg(config->kernel, 4, sizeof(cl_mem), ACC_OPENCL_MEM(dev_c_data)),
          "set C-matrix argument of SMM-kernel", result);
        ACC_OPENCL_CHECK(clEnqueueNDRangeKernel(*ACC_OPENCL_STREAM(stack_stream),
          config->kernel, 1/*work_dim*/, NULL, &work_size, &config->wgsize, 0, NULL, NULL),
          "launch SMM-kernel", result);
      }
    }
    else if (0 != stack_size) { /* inhomogeneous or large */
      result = EXIT_FAILURE; /* TODO: signal host-fallback */
    }
  }
  else {
    result = EXIT_FAILURE;
  }
  ACC_OPENCL_RETURN(result);
}

#if defined(__cplusplus)
}
#endif

#endif /*__OPENCL*/
