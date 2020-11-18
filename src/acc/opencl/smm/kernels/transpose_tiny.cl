/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

kernel void FN(global const int *restrict trs_stack, int trs_offset, global T *restrict matrix)
{
  /* offset in the transpose-stack that this block ID should handle */
  const int offset = trs_stack[trs_offset+get_group_id(0)];
  /* matrix according to the index (transpose-stack) */
  global T *const restrict mat = matrix + offset;
  /* local memory buffer */
  private T buf[SM*SN];

  /* copy matrix elements into local buffer */
  for (int m = 0; m < SM; ++m) {
    for (int n = 0; n < SN; ++n) buf[SN*m+n] = mat[SN*m+n];
  }

  /* overwrite matrix elements (gather) */
  for (int m = 0; m < SM; ++m) {
    for (int n = 0; n < SN; ++n) mat[SN*m+n] = buf[SM*n+m];
  }
}
