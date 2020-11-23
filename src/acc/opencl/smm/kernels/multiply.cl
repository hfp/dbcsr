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
  global const T *const restrict amat = matrix + param_base[0];
  global const T *const restrict bmat = matrix + param_base[1];
  global T *const restrict cmat = matrix + param_base[2];
  KIND T buf[SM*SN];

  const int index = get_local_id(0);
  switch (get_local_size(0)) {
    case SM: {
      const int m = index;
      /* copy matrix elements into local buffer */
      for (int n = 0; n < SN; ++n) buf[SN*m+n] = mat[SN*m+n];
      barrier(CLK_LOCAL_MEM_FENCE);
      /* overwrite matrix elements (gather) */
      for (int n = 0; n < SN; ++n) mat[SN*m+n] = buf[SM*n+m];
    } break;
    default: if (index < SM) {
      const int nblocks = max(SM / (int)get_local_size(0), 1);
      const int base = nblocks * index;
      /* copy matrix elements into local buffer */
      for (int m = base; m < (base + nblocks); ++m) {
        for (int n = 0; n < SN; ++n) buf[SN*m+n] = mat[SN*m+n];
      }
      barrier(CLK_LOCAL_MEM_FENCE);
      /* overwrite matrix elements (gather) */
      for (int m = base; m < (base + nblocks); ++m) {
        for (int n = 0; n < SN; ++n) mat[SN*m+n] = buf[SM*n+m];
      }
    }
  }
}
