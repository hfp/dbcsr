/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

kernel void FN(global const int *restrict param_stack,
  global const T *restrict amat, global const T *restrict bmat, global T *restrict cmat)
{
  global const int *restrict param_base = param_stack + get_group_id(0) * 3;
  /* indexes given by param_stack are one-based */
  global const T *const restrict a = amat + param_base[0] - 1;
  global const T *const restrict b = bmat + param_base[1] - 1;
  global T *const restrict c = cmat + param_base[2] - 1;
  local T buf[SK*SN], cuf[SM*SN];

  const int index = get_local_id(0);
  switch (get_local_size(0)) {
    case SN: {
      const int n = index;
      for (int k = 0; k < SK; ++k) buf[SN*k+n] = b[SN*k+n];
      barrier(CLK_LOCAL_MEM_FENCE);
      for (int m = 0; m < SM; ++m) {
        T r = 0;
        for (int k = 0; k < SK; ++k) r += a[SM*k+m] * buf[SN*k+n];
        cuf[SM*n+m] = r;
      }
      barrier(CLK_LOCAL_MEM_FENCE);
      /* TODO: atomic commit */
      for (int m = 0; m < SM; ++m) c[SM*n+m] = cuf[SM*n+m];
    } break;
    default: if (index < SN) {
    }
  }
}
