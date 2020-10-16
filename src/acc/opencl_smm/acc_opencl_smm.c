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

acc_bool_t libsmm_acc_is_thread_safe(void)
{
  return 1/*true*/;
}


int libsmm_acc_transpose(const int* dev_trs_stack, int offset, int stack_size,
  void* dev_data, libsmm_acc_data_t datatype, int m, int n, int max_kernel_dim, void* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_trs_stack && NULL != dev_data) || 0 == stack_size);
  if (0 != stack_size) {
    switch (datatype) {
      case dbcsr_type_real_8: {
        result = acc_opencl_dbatchtrans(dev_trs_stack, offset, stack_size,
          (double*)dev_data, m, n, max_kernel_dim, stream);
      } break;
      case dbcsr_type_real_4: {
        result = acc_opencl_sbatchtrans(dev_trs_stack, offset, stack_size,
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
  assert((nparams * sizeof(int)) == sizeof(libsmm_acc_smmstack_t));
  if (0 != stack_size && def_mnk/*homogeneous*/) {
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
  return result;
}

#if defined(__cplusplus)
}
#endif
