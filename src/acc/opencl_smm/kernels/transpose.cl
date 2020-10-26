/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

/* work group size must be at least M*N */
__kernel void FN(__global int* trs_stack, int trs_offset, __global T* matrix)
{
  __local T buf[SM*SN];

  /* offset in the transpose-stack that this block ID should handle */
  const int offset = trs_stack[trs_offset+get_group_id(0)];
  /* matrix according to the index (transpose-stack) */
  __global T *const mat = matrix + offset;

  /* copy matrix elements into a local buffer */
  for (int i = get_local_id(0); i < (SM*SN); i += SM) {
    buf[i] = mat[i];
  }
  barrier(CLK_LOCAL_MEM_FENCE);

  for (int i = get_local_id(0); i < (SM*SN); i += SM) {
    /* compute old row and column index of matrix element */
    const int c = i / SN, r = i % SN;
    /* overwrite the matrix element (gather) */
    mat[i] = buf[r*SM+c];
  }
}
