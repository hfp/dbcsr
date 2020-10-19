/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

__kernel void FN(__global int* trs_stack, int trs_offset, __global T* mat)
{
  __local T buf[SM*SN];

  /* Get the offset in the transpose-stack that this block ID should handle */
  const int offset = trs_stack[trs_offset+get_group_id(0)];
  /* Load matrix elements into a temporary buffer */
  event_t e = async_work_group_copy(buf, mat + offset, SM * SN, 0/*NULL*/);
  wait_group_events(1, &e);

  /* Loop over elements of the matrix to be overwritten */
  for (int i = get_local_id(0); i < (SM * SN); i += get_local_size(0)) {
    /* Compute old row and column index of matrix element */
    const int c_out = i / SN, r_out = i - c_out * SN /* i % SN */;
     /* Compute the corresponding old 1D index of matrix element */
    const int idx = r_out * SM + c_out;
    /* Overwrite the matrix element */
    mat[offset+i] = buf[idx];
  }
}
