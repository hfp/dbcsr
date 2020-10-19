/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

__kernel void F(__global int* trs_stack, int trs_offset, __global T* mat)
{
  __local T buf[M*N];

  /* Get the offset in the transpose-stack that this block ID should handle */
  const int offset = trs_stack[trs_offset+get_group_id(0)];

  /* Loop over M*N matrix elements */
  for (int i = get_local_id(0); i < (M * N); i += get_local_size(0)) {
    /* Load matrix elements into a temporary buffer */
    buf[i] = mat[offset+i];
  }
  barrier(CLK_LOCAL_MEM_FENCE);

  /* Loop over elements of the matrix to be overwritten */
  for (int i = get_local_id(0); i < (M * N); i += get_local_size(0)) {
    /* Compute old row and column index of matrix element */
    const int c_out = i / N, r_out = i - c_out * N /* i % N */;
     /* Compute the corresponding old 1D index of matrix element */
    const int idx = r_out * M + c_out;
    /* Overwrite the matrix element */
    mat[offset+i] = buf[idx];
  }
}
