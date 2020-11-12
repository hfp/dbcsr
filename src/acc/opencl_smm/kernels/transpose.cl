/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

__kernel void FN(__global int* trs_stack, int trs_offset, __global T* matrix)
{
  /* offset in the transpose-stack that this block ID should handle */
  const int offset = trs_stack[trs_offset+get_group_id(0)];
  /* matrix according to the index (transpose-stack) */
  __global T *const mat = matrix + offset;
  const int m = get_local_id(0);
#if defined(INPLACE) && (SM == SN)
  for (int n = 0; n < m; ++n) {
    const int i = SM * n + m;
    const int j = SN * m + n;
    const T tmp = mat[i];
    mat[i] = mat[j];
    mat[j] = tmp;
  }
#else
  __local T buf[SM*SN];
  /* copy matrix elements into local buffer */
  for (int n = 0; n < SN; ++n) buf[SN*m+n] = mat[SN*m+n];
  barrier(CLK_LOCAL_MEM_FENCE);
  /* overwrite matrix elements (gather) */
  for (int n = 0; n < SN; ++n) mat[SN*m+n] = buf[SM*n+m];
#endif
}
