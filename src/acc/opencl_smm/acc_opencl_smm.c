/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "libsmm_omp.h"
#include <assert.h>

#if !defined(ACC_OPENCL_SMM_MAXLINELEN)
# define ACC_OPENCL_SMM_MAXLINELEN 128
#endif

#if defined(__STDC_VERSION__) && (199901L <= __STDC_VERSION__ || defined(__GNUC__))
# define ACC_OPENCL_SMM_SNPRINTF(S, N, ...) snprintf(S, N, __VA_ARGS__)
#else
# define ACC_OPENCL_SMM_SNPRINTF(S, N, ...) sprintf((S) + /*unused*/(N) * 0, __VA_ARGS__)
#endif


#if defined(__cplusplus)
extern "C" {
#endif

int acc_opencl_template(FILE* source, char* lines[], int max_nlines, int skip_lines,
  const char* type, const int params[], int nparams)
{
  int nlines = 0;
  if (((NULL != source && NULL != lines && 0 < max_nlines) || 0 >= max_nlines)
    && 0 <= skip_lines && ((NULL != type && 0 != *type) || NULL == type)
    && ((NULL != params && 0 < nparams) || 0 >= nparams))
  {
    if (0 < max_nlines) {
      const int* param = params;
      lines[0] = (char*)malloc(max_nlines * sizeof(line));
      while (nlines < max_nlines && NULL != lines[nlines]
        && NULL != fgets(lines[nlines], ACC_OPENCL_SMM_MAXLINELEN, source))
      {
        if (0 == skip_lines) {
          char* subst = ((NULL != type || 0 < nparams) ? strchr(lines[nlines], '%') : NULL);
          int next = nlines + 1;
          if (next < max_nlines) lines[next] = lines[nlines] + ACC_OPENCL_SMM_MAXLINELEN;
          while (NULL != subst) {
            int maxlen, len;
            if (NULL != type && 's' == subst[1]) {
              subst = (0 < nparams ? strchr(lines[nlines], '%') : NULL);
              maxlen = ACC_OPENCL_SMM_MAXLINELEN - (subst - lines[nlines]);
              len = (ACC_OPENCL_SMM_SNPRINTF(subst, maxlen, type));
              type = NULL;
            }
            else if (0 < nparams && 'i' == subst[1]) {
              subst = (NULL != type ? strchr(lines[nlines], '%') : NULL);
              maxlen = ACC_OPENCL_SMM_MAXLINELEN - (subst - lines[nlines]);
              len = ACC_OPENCL_SMM_SNPRINTF(subst, maxlen, *param);
              --nparams;
              ++param;
            }
            else {
              maxlen = len = 0;
            }
            if (0 > len || len >= maxlen) {
              free(lines[0]);
              lines[0] = NULL;
              subst = NULL;
              next = 0;
            }
          }
          nlines = next;
        }
        else { /* skip line */
          --skip_lines;
        }
      }
    }
  }
  return nlines;
}


acc_bool_t libsmm_acc_is_thread_safe(void)
{
  return 1/*true*/;
}


int libsmm_acc_transpose(const int* dev_trs_stack, int offset, int nblks,
  void* dev_data, libsmm_acc_data_t datatype, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_trs_stack && NULL != dev_data) || 0 == nblks);
  if (0 != nblks) {
    switch (datatype) {
      case dbcsr_type_real_8: {
        result = acc_opencl_transpose_d(dev_trs_stack, offset, nblks,
          (double*)dev_data, m, n, max_kernel_dim, stream);
      } break;
      case dbcsr_type_real_4: {
        result = acc_opencl_transpose_s(dev_trs_stack, offset, nblks,
          (float*)dev_data, m, n, max_kernel_dim, stream);
      } break;
      default: {
        result = EXIT_FAILURE;
      }
    }
  }
  return result;
}


int libsmm_acc_process(const int* host_param_stack, const int* dev_param_stack, int stack_size,
  int nparams, libsmm_acc_data_t datatype, const void* dev_a_data, const void* dev_b_data, void* dev_c_data,
  int m_max, int n_max, int k_max, int max_kernel_dim, acc_bool_t def_mnk, void* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_param_stack && NULL != dev_a_data && NULL != dev_b_data && NULL != dev_c_data) || 0 == stack_size);
  assert((nparams * sizeof(int)) == sizeof(libsmm_acc_stackdesc_t));
  if (0 != stack_size && def_mnk/*homogeneous*/) {
    switch (datatype) {
      case dbcsr_type_real_8: {
        result = acc_opencl_process_d(host_param_stack, dev_param_stack, stack_size,
          (const double*)dev_a_data, (const double*)dev_b_data, (double*)dev_c_data,
          m_max, n_max, k_max, max_kernel_dim, stream);
      } break;
      case dbcsr_type_real_4: {
        result = acc_opencl_process_s(host_param_stack, dev_param_stack, stack_size,
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
  return result;
}

#if defined(__cplusplus)
}
#endif
