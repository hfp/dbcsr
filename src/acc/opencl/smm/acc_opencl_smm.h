/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#ifndef ACC_OPENCL_SMM_H
#define ACC_OPENCL_SMM_H

#include "../../acc_libsmm.h"
#include "../acc_opencl.h"

#if defined(__LIBXSMM)
# include <libxsmm.h>
#else
# error OpenCL backend currently depends on LIBXSMM!
#endif

#if !defined(ACC_OPENCL_SMM_PERMIT_TRANSPOSE_INPLACE) && 0
# define ACC_OPENCL_SMM_PERMIT_TRANSPOSE_INPLACE
#endif

#if !defined(ACC_OPENCL_SMM_PERMIT_TRANSPOSE_TINY) && 0
# define ACC_OPENCL_SMM_PERMIT_TRANSPOSE_TINY 16
#endif

#endif /*ACC_OPENCL_SMM_H*/
