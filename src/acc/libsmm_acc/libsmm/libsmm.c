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

int libsmm_acc_process(void* param_stack, int stack_size,
  int nparams, int datatype, void* a_data, void* b_data, void* c_data,
  int m_max, int n_max, int k_max, int def_mnk, acc_stream_t stream)
{
  int result;
  if (0 != def_mnk) {
    switch (datatype) {
      case DBCSR_DATATYPE_F64: {
        result = EXIT_FAILURE; /* TODO */
      } break;
      case DBCSR_DATATYPE_F32: {
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


int libsmm_acc_transpose(void* trs_stack, int offset, int nblks,
  void* buffer, int datatype, int m, int n, acc_stream_t stream)
{
  int result;
  switch (datatype) {
    case DBCSR_DATATYPE_F64: {
      result = EXIT_FAILURE; /* TODO */
    } break;
    case DBCSR_DATATYPE_F32: {
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
