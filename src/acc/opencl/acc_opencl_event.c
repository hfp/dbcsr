/*------------------------------------------------------------------------------------------------*/
/* Copyright (C) by the DBCSR developers group - All rights reserved                              */
/* This file is part of the DBCSR library.                                                        */
/*                                                                                                */
/* For information on the license, see the LICENSE file.                                          */
/* For further information please visit https://dbcsr.cp2k.org                                    */
/* SPDX-License-Identifier: GPL-2.0+                                                              */
/*------------------------------------------------------------------------------------------------*/
#if defined(__OPENCL)
#  include "acc_opencl.h"

#  if !defined(ACC_OPENCL_EVENT_FLUSH) && 0
#    define ACC_OPENCL_EVENT_FLUSH
#  endif


#  if defined(__cplusplus)
extern "C" {
#  endif

int c_dbcsr_acc_event_create(void** event_p) {
  int result = EXIT_SUCCESS;
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  int routine_handle;
  static const char* const routine_name_ptr = LIBXSMM_FUNCNAME;
  static const int routine_name_len = (int)sizeof(LIBXSMM_FUNCNAME) - 1;
  c_dbcsr_timeset((const char**)&routine_name_ptr, &routine_name_len, &routine_handle);
#  endif
  assert(NULL != c_dbcsr_acc_opencl_config.events && NULL != event_p);
  *event_p = c_dbcsr_acc_opencl_pmalloc((void**)c_dbcsr_acc_opencl_config.events, &c_dbcsr_acc_opencl_config.nevents);
  if (NULL != *event_p) *(cl_event*)*event_p = NULL;
  else result = EXIT_FAILURE;
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  c_dbcsr_timestop(&routine_handle);
#  endif
  ACC_OPENCL_RETURN(result);
}


int c_dbcsr_acc_event_destroy(void* event) {
  int result = EXIT_SUCCESS;
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  int routine_handle;
  static const char* const routine_name_ptr = LIBXSMM_FUNCNAME;
  static const int routine_name_len = (int)sizeof(LIBXSMM_FUNCNAME) - 1;
  c_dbcsr_timeset((const char**)&routine_name_ptr, &routine_name_len, &routine_handle);
#  endif
  if (NULL != event) {
    const cl_event clevent = *ACC_OPENCL_EVENT(event);
    assert(NULL != c_dbcsr_acc_opencl_config.events);
    c_dbcsr_acc_opencl_pfree(event, (void**)c_dbcsr_acc_opencl_config.events, &c_dbcsr_acc_opencl_config.nevents);
    if (NULL != clevent) result = clReleaseEvent(clevent);
  }
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  c_dbcsr_timestop(&routine_handle);
#  endif
  ACC_OPENCL_RETURN(result);
}


int c_dbcsr_acc_stream_wait_event(void* stream, void* event) { /* wait for an event (device-side) */
  int result = EXIT_SUCCESS;
  const c_dbcsr_acc_opencl_stream_t* str = NULL;
  cl_event clevent = NULL;
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  int routine_handle;
  static const char* const routine_name_ptr = LIBXSMM_FUNCNAME;
  static const int routine_name_len = (int)sizeof(LIBXSMM_FUNCNAME) - 1;
  c_dbcsr_timeset((const char**)&routine_name_ptr, &routine_name_len, &routine_handle);
#  endif
#  if defined(ACC_OPENCL_STREAM_NULL)
  str = (NULL != stream ? ACC_OPENCL_STREAM(stream) : c_dbcsr_acc_opencl_stream_default());
#  else
  str = ACC_OPENCL_STREAM(stream);
#  endif
  assert(NULL != str && NULL != str->queue && NULL != event);
  clevent = *ACC_OPENCL_EVENT(event);
  if (NULL != clevent) {
#  if defined(CL_VERSION_1_2)
    result = clEnqueueBarrierWithWaitList(str->queue, 1, &clevent, NULL);
#  else
    result = clEnqueueWaitForEvents(str->queue, 1, &clevent);
#  endif
  }
  /*else result = EXIT_FAILURE;*/
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  c_dbcsr_timestop(&routine_handle);
#  endif
  ACC_OPENCL_RETURN(result);
}


int c_dbcsr_acc_event_record(void* event, void* stream) {
  int result = EXIT_SUCCESS;
  const c_dbcsr_acc_opencl_stream_t* str = NULL;
  cl_event clevent = NULL;
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  int routine_handle;
  static const char* const routine_name_ptr = LIBXSMM_FUNCNAME;
  static const int routine_name_len = (int)sizeof(LIBXSMM_FUNCNAME) - 1;
  c_dbcsr_timeset((const char**)&routine_name_ptr, &routine_name_len, &routine_handle);
#  endif
#  if defined(ACC_OPENCL_STREAM_NULL)
  str = (NULL != stream ? ACC_OPENCL_STREAM(stream) : c_dbcsr_acc_opencl_stream_default());
#  else
  str = ACC_OPENCL_STREAM(stream);
#  endif
  assert(NULL != str && NULL != str->queue && NULL != event);
  clevent = *ACC_OPENCL_EVENT(event);
  if (NULL != clevent) {
    ACC_OPENCL_EXPECT(EXIT_SUCCESS == clReleaseEvent(clevent));
#  if !defined(NDEBUG)
    clevent = NULL;
#  endif
  }
#  if defined(CL_VERSION_1_2)
  result = clEnqueueMarkerWithWaitList(str->queue, 0, NULL, &clevent);
#  else
  result = clEnqueueMarker(str->queue, &clevent);
#  endif
  if (EXIT_SUCCESS == result) {
    assert(NULL != clevent);
#  if defined(ACC_OPENCL_EVENT_FLUSH)
    result = clFlush(str->queue);
    *(cl_event*)event = (EXIT_SUCCESS == result ? clevent : NULL);
#  else
    *(cl_event*)event = clevent;
#  endif
  }
  else *(cl_event*)event = NULL;
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  c_dbcsr_timestop(&routine_handle);
#  endif
  ACC_OPENCL_RETURN(result);
}


int c_dbcsr_acc_event_query(void* event, c_dbcsr_acc_bool_t* has_occurred) {
  cl_int status = CL_COMPLETE;
  int result;
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  int routine_handle;
  static const char* const routine_name_ptr = LIBXSMM_FUNCNAME;
  static const int routine_name_len = (int)sizeof(LIBXSMM_FUNCNAME) - 1;
  c_dbcsr_timeset((const char**)&routine_name_ptr, &routine_name_len, &routine_handle);
#  endif
  assert(NULL != event && NULL != has_occurred);
  result = clGetEventInfo(*ACC_OPENCL_EVENT(event), CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, NULL);
  if (EXIT_SUCCESS == result && 0 <= status) *has_occurred = (CL_COMPLETE == status ? 1 : 0);
  else { /* error state */
    result = EXIT_SUCCESS; /* soft-error */
    *has_occurred = 1;
  }
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  c_dbcsr_timestop(&routine_handle);
#  endif
  ACC_OPENCL_RETURN(result);
}


int c_dbcsr_acc_event_synchronize(void* event) { /* waits on the host-side */
  int result = EXIT_SUCCESS;
  cl_event clevent;
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  int routine_handle;
  static const char* const routine_name_ptr = LIBXSMM_FUNCNAME;
  static const int routine_name_len = (int)sizeof(LIBXSMM_FUNCNAME) - 1;
  c_dbcsr_timeset((const char**)&routine_name_ptr, &routine_name_len, &routine_handle);
#  endif
  assert(NULL != event);
  clevent = *ACC_OPENCL_EVENT(event);
  if (NULL != clevent) {
    result = clWaitForEvents(1, &clevent);
  }
  else if (3 <= c_dbcsr_acc_opencl_config.verbosity || 0 > c_dbcsr_acc_opencl_config.verbosity) {
    fprintf(stderr, "WARN ACC/OpenCL: empty event discovered.\n");
  }
#  if defined(__DBCSR_ACC) && defined(ACC_OPENCL_PROFILE)
  c_dbcsr_timestop(&routine_handle);
#  endif
  ACC_OPENCL_RETURN(result);
}

#  if defined(__cplusplus)
}
#  endif

#endif /*__OPENCL*/
