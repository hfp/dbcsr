/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/
#ifndef DBCSR_ACC_LIBSMM_H
#define DBCSR_ACC_LIBSMM_H

#include "../include/libsmm_acc.h"
#include "../../openmp/acc_openmp.h"

typedef enum dbcsr_datatype {
  DBCSR_DATATYPE_F64 = 3,
  DBCSR_DATATYPE_F32 = 1,
  DBCSR_DATATYPE_UNSUPPORTED = -1
} dbcsr_datatype;

#endif /*DBCSR_ACC_LIBSMM_H*/
