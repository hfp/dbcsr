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

int libsmm_acc_libcusmm_is_thread_safe(void)
{
  return 1/*true*/;
}


int libsmm_acc_transpose(const int* dev_trs_stack, int offset, int nblks,
  void* dev_data, acc_data_t datatype, int m, int n, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_trs_stack && NULL != dev_data) || 0 == nblks);
  if (0 != nblks) {
#if defined(ACC_OPENMP_OFFLOAD)
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        deps->args[0].const_ptr = dev_trs_stack;
        deps->args[1].i32 = offset;
        deps->args[2].i32 = nblks;
        deps->args[3].ptr = dev_data;
        deps->args[4].i32 = datatype;
        deps->args[5].i32 = m;
        deps->args[6].i32 = n;
        deps->args[7].ptr = stream;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          int tid = 0;
          for (; tid < nthreads && EXIT_SUCCESS == result; ++tid) {
            acc_openmp_depend_t *const di = &deps[tid];
            const int* /*const*/ index = (const int*)di->args[0].const_ptr;
            void* /*const*/ data = di->args[3].ptr;
            const char *const id = di->in, *const od = di->out;
            switch (datatype) {
              case ACC_DATA_F64: {
#               pragma omp target depend(in:ACC_OPENMP_DEP(id)) depend(out:ACC_OPENMP_DEP(od)) nowait is_device_ptr(index,data)
                result = libsmm_acc_transpose_d(index, deps->args[1].i32/*offset*/, deps->args[2].i32/*nblks*/,
                  (double*)data, deps->args[5].i32/*m*/, deps->args[6].i32/*n*/);
              } break;
              case ACC_DATA_F32: {
#               pragma omp target depend(in:ACC_OPENMP_DEP(id)) depend(out:ACC_OPENMP_DEP(od)) nowait is_device_ptr(index,data)
                result = libsmm_acc_transpose_s(index, deps->args[1].i32/*offset*/, deps->args[2].i32/*nblks*/,
                  (float*)data, deps->args[5].i32/*m*/, deps->args[6].i32/*n*/);
              } break;
              default: {
                acc_openmp_stream_t *const s = (acc_openmp_stream_t*)deps->args[7].ptr;
                s->status = EXIT_FAILURE;
                result = EXIT_FAILURE;
              }
            }
            (void)(id); (void)(od); /* suppress incorrect warning */
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    switch (datatype) {
      case ACC_DATA_F64: {
        result = libsmm_acc_transpose_d(dev_trs_stack, offset, nblks, (double*)dev_data, m, n);
      } break;
      case ACC_DATA_F32: {
        result = libsmm_acc_transpose_s(dev_trs_stack, offset, nblks, (float*)dev_data, m, n);
      } break;
      default: {
        acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
        s->status = EXIT_FAILURE;
        result = EXIT_FAILURE;
      }
    }
  }
  return result;
}


int libsmm_acc_process(const libsmm_acc_stack_descriptor_type* dev_param_stack, int stack_size,
  int nparams, acc_data_t datatype, const void* dev_a_data, const void* dev_b_data, void* dev_c_data,
  int m_max, int n_max, int k_max, acc_bool_t def_mnk, acc_stream_t* stream)
{
  int result = EXIT_SUCCESS;
  assert((NULL != dev_param_stack && NULL != dev_a_data && NULL != dev_b_data && NULL != dev_c_data) || 0 == stack_size);
  assert(7 == nparams);
  if (0 != stack_size && def_mnk/*homogeneous*/) {
#if defined(ACC_OPENMP_OFFLOAD)
    const int ndevices = omp_get_num_devices();
    if (0 < ndevices) {
      acc_openmp_depend_t* deps;
      result = acc_openmp_stream_depend(stream, &deps);
      if (EXIT_SUCCESS == result) {
        deps->args[0].const_ptr = dev_param_stack;
        deps->args[1].i32 = stack_size;
        deps->args[2].i32 = datatype;
        deps->args[3].const_ptr = dev_a_data;
        deps->args[4].const_ptr = dev_b_data;
        deps->args[5].ptr = dev_c_data;
        deps->args[6].i32 = m_max;
        deps->args[7].i32 = n_max;
        deps->args[8].i32 = k_max;
        deps->args[9].ptr = stream;
#       pragma omp barrier
#       pragma omp master
        { const int nthreads = omp_get_num_threads();
          int tid = 0;
          for (; tid < nthreads && EXIT_SUCCESS == result; ++tid) {
            acc_openmp_depend_t *const di = &deps[tid];
            const libsmm_acc_stack_descriptor_type* /*const*/ params = (const libsmm_acc_stack_descriptor_type*)di->args[0].const_ptr;
            const void * /*const*/ a_data = di->args[3].const_ptr, * /*const*/ b_data = di->args[4].const_ptr;
            void* /*const*/ c_data = di->args[5].ptr;
            const char *const id = di->in, *const od = di->out;
            switch (datatype) {
              case ACC_DATA_F64: {
#               pragma omp target depend(in:ACC_OPENMP_DEP(id)) depend(out:ACC_OPENMP_DEP(od)) nowait is_device_ptr(params,a_data,b_data,c_data)
                result = libsmm_acc_process_d(params, deps->args[1].i32/*stack_size*/, nparams,
                  (const double*)a_data, (const double*)b_data, (double*)c_data,
                  deps->args[6].i32/*m_max*/, deps->args[7].i32/*n_max*/, deps->args[8].i32/*k_max*/);
              } break;
              case ACC_DATA_F32: {
#               pragma omp target depend(in:ACC_OPENMP_DEP(id)) depend(out:ACC_OPENMP_DEP(od)) nowait is_device_ptr(params,a_data,b_data,c_data)
                result = libsmm_acc_process_s(params, deps->args[1].i32/*stack_size*/, nparams,
                  (const float*)a_data, (const float*)b_data, (float*)c_data,
                  deps->args[6].i32/*m_max*/, deps->args[7].i32/*n_max*/, deps->args[8].i32/*k_max*/);
              } break;
              default: {
                acc_openmp_stream_t *const s = (acc_openmp_stream_t*)deps->args[9].ptr;
                s->status = EXIT_FAILURE;
                result = EXIT_FAILURE;
              }
            }
            (void)(id); (void)(od); /* suppress incorrect warning */
          }
        }
#       pragma omp barrier
      }
    }
    else
#endif
    switch (datatype) {
      case ACC_DATA_F64: {
        result = libsmm_acc_process_d(dev_param_stack, stack_size, nparams,
          (const double*)dev_a_data, (const double*)dev_b_data, (double*)dev_c_data,
          m_max, n_max, k_max);
      } break;
      case ACC_DATA_F32: {
        result = libsmm_acc_process_s(dev_param_stack, stack_size, nparams,
          (const float*)dev_a_data, (const float*)dev_b_data, (float*)dev_c_data,
          m_max, n_max, k_max);
      } break;
      default: {
        acc_openmp_stream_t *const s = (acc_openmp_stream_t*)stream;
        s->status = EXIT_FAILURE;
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
