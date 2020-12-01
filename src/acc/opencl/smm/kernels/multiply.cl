/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

inline void atomic_global_add1(global volatile T* dst, T inc)
{
  union { TA a; T f; } old_val, new_val;
  do {
    old_val.f = *dst;
    new_val.f = old_val.f + inc;
  } while (old_val.a != atom_cmpxchg((global volatile TA*)dst, old_val.a, new_val.a));
}


inline void atomic_global_addn(global volatile T* dst, local const T* mat, int n)
{
  for (int m = 0; m < SM; ++m) {
    atomic_global_add1(&dst[SN*m+n], mat[SM*n+m]);
  }
}


kernel void FN(global const int *restrict param_stack, global volatile int *restrict locks,
  global const T *restrict amat, global const T *restrict bmat, global T *restrict cmat)
{
  const int gid = get_group_id(0);
  global const int *const restrict param_base = param_stack + gid * 3;
  /* indexes given by param_stack are one-based */
  global const T *const restrict awg = amat + param_base[0] - 1;
  global const T *const restrict bwg = bmat + param_base[1] - 1;
  global T *const restrict cwg = cmat + param_base[2] - 1;
  local T a[SM*SK], c[SM*SN];
  T b[SK];

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
        T r = 0;
        for (int k = 0; k < SK; ++k) r += a[SK*m+k] * b[k];
        c[SM*n+m] = r;
      }
      barrier(CLK_LOCAL_MEM_FENCE);
      atomic_global_addn(cwg, c, n);
    } break;
    default: if (index < SN) {
    }
  }
}
