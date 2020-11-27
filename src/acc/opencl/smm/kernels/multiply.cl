/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

inline void add_atomic(global volatile T* address, T inc)
{
  union { TA a; T f; } old_val, new_val;
  do {
    old_val.f = *address;
    new_val.f = old_val.f + inc;
  } while (old_val.a != atom_cmpxchg((global volatile TA*)address, old_val.a, new_val.a));
}


kernel void FN(global const int *restrict param_stack,
  global const T *restrict amat, global const T *restrict bmat, global T *restrict cmat)
{
  global const int *const restrict param_base = param_stack + get_group_id(0) * 3;
  /* indexes given by param_stack are one-based */
  global const T *const restrict awg = amat + param_base[0] - 1;
  global const T *const restrict bwg = bmat + param_base[1] - 1;
  global T *const restrict c = cmat + param_base[2] - 1;
  local T a[SM*SK];
  T b[SK];

  const int size = get_local_size(0);
  const int index = get_local_id(0);
  switch (size) {
    case SN: {
      const int n = index, nblocks = (SM * SK + size - 1) / size;
      const int i0 = n * nblocks, i1 = min(i0 + nblocks, SM * SK);
      for (int i = i0; i < i1; ++i) a[i] = awg[i];
      barrier(CLK_LOCAL_MEM_FENCE);
      for (int k = 0; k < SK; ++k) b[k] = bwg[SN*k+n];
      for (int m = 0; m < SM; ++m) {
        T r = 0;
        for (int k = 0; k < SK; ++k) r += a[SM*k+m] * b[k];
        add_atomic(&c[SM*n+m], r);
      }
    } break;
    default: if (index < SN) {
      barrier(CLK_LOCAL_MEM_FENCE);
    }
    else barrier(CLK_LOCAL_MEM_FENCE);
  }
}
