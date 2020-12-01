/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

inline void atomic_global_add(global volatile T* dst, T inc)
{
  union { TA a; T f; } old_val, try_val, new_val;
  for (old_val.f = *dst;;) {
    try_val.f = old_val.f + inc;
    new_val.a = atom_cmpxchg((global volatile TA*)dst, old_val.a, try_val.a);
    if (old_val.a == new_val.a) break;
    old_val.a = new_val.a;
  }
}


kernel void FN(global const int *restrict param_stack, global volatile int *restrict locks,
  global const T *restrict amat, global const T *restrict bmat, global T *restrict cmat)
{
  const int gid = get_group_id(0);
  /* indexes given by param_stack are one-based */
  const int3 idx = *(global const int3*)(param_stack + gid * 3) - 1;
  global const T *const restrict awg = amat + idx.s0;
  global const T *const restrict bwg = bmat + idx.s1;
  global T *const restrict cwg = cmat + idx.s2;
  local T a[SM*SK];
  T b[SK], c[SM];

  const int size = get_local_size(0);
  const int index = get_local_id(0);
  switch (size) {
    case SN: {
      const int n = index, nblocks = (SM * SK + size - 1) / size;
      const int i0 = n * nblocks, i1 = min(i0 + nblocks, SM * SK);
      /* split work among WG (a[m,k] does not depend on WG-index) */
      for (int i = i0; i < i1; ++i) a[i] = awg[i];
      barrier(CLK_LOCAL_MEM_FENCE);
      for (int k = 0; k < SK; ++k) b[k] = bwg[SK*n+k];
      for (int m = 0; m < SM; ++m) {
        for (int k = 0; k < SK; ++k) c[m] += a[SK*m+k] * b[k];
      }
      for (int m = 0; m < SM; ++m) {
        atomic_global_add(&cwg[SN*m+n], c[m]);
      }
    } break;
    default: if (index < SN) {
    }
  }
}
