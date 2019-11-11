/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#include "libsmm.h"
#include <stdlib.h>
#include <assert.h>


#if defined(__cplusplus)
extern "C" {
#endif

int libsmm_acc_process(const libsmm_acc_stack_descriptor_type* param_stack, int stack_size,
  int nparams, acc_data_t datatype, void* a_data, void* b_data, void* c_data,
  int m_max, int n_max, int k_max, acc_bool_t def_mnk, acc_stream_t* stream)
{
  int result;
#if defined(NDEBUG)
  (void)(nparams); /* unused */
#endif
  assert(7 == nparams);
  if (def_mnk) {
    switch (datatype) {
      case ACC_DATA_F64: {
        result = EXIT_FAILURE; /* TODO */
      } break;
      case ACC_DATA_F32: {
        result = EXIT_FAILURE; /* TODO */
      } break;
      default: result = EXIT_FAILURE;
    }
  }
  else { /* TODO: inhomogeneous stack */
    result = EXIT_FAILURE;
  }
  return result;
}


int libsmm_acc_transpose(const int* trs_stack, int offset, int nblks,
  void* data, acc_data_t datatype, int m, int n, acc_stream_t* stream)
{
  int result;
  switch (datatype) {
    case ACC_DATA_F64: {
      result = EXIT_FAILURE; /* TODO */
    } break;
    case ACC_DATA_F32: {
      result = EXIT_FAILURE; /* TODO */
    } break;
    default: result = EXIT_FAILURE;
  }
  return result;
}


int libsmm_acc_libcusmm_is_thread_safe(void)
{
  return 1/*true*/;
}

#if defined(__cplusplus)
}
#endif
