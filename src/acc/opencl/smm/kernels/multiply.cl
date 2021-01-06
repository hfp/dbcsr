/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

__attribute__((always_inline))
inline void atomic_add_global_cmpxchg(global volatile T* dst, T inc)
{
  union { TA a; T f; } old_val, try_val, new_val = { .f = *dst };
  do {
    old_val.a = new_val.a;
    try_val.f = old_val.f + inc;
    new_val.a = CMPXCHG((global volatile TA*)dst, old_val.a, try_val.a);
  } while (old_val.a != new_val.a);
}


__attribute__((always_inline))
inline void atomic_add_global_xchg(global volatile T* dst, T inc)
{
  union { TA a; T f; } old_val = { .f = inc }, try_val, new_val = { .f = 0 };
  do {
    try_val.a = XCHG((global volatile TA*)dst, new_val.a);
    try_val.f += old_val.f;
    old_val.a = XCHG((global volatile TA*)dst, try_val.a);
  } while (old_val.a != new_val.a);
}


kernel void FN(GLOBAL const int *restrict param_stack,
  GLOBAL const T *restrict amat, GLOBAL const T *restrict bmat,
  global T *restrict cmat)
{
  const int gid = get_group_id(0);
  const int idx = get_local_id(0);
  const int nbn = (SN + BN - 1) / BN;

  GLOBAL const int *const restrict param_base = param_stack + gid * 3;
  /* indexes given by param_stack are one-based */
  const int ai = param_base[0] - 1;
  const int bi = param_base[1] - 1;
  const int ci = param_base[2] - 1;

  local T a[SM*SK], b[SK*SN];
  T c[BM*BN] = { 0 };

  const int im = idx / nbn, in = idx - im * nbn;
  const int m0 = im * BM, m1 = min(m0 + BM, SM);
  const int n0 = in * BN, n1 = min(n0 + BN, SN);

  { /* transpose A-matrix into local buffer */
    GLOBAL const T *const restrict awg = amat + ai;
    for (int m = m0; m < m1; ++m) {
      for (int k = 0; k < SK; ++k) a[SM*m+k] = awg[SM*k+m];
    }
  }

  { /* copy B-matrix into local buffer */
    GLOBAL const T *const restrict bwg = bmat + bi;
    for (int k = 0; k < SK; ++k) {
      for (int n = n0; n < n1; ++n) b[SN*k+n] = bwg[SN*k+n];
    }
  }

  barrier(CLK_LOCAL_MEM_FENCE);
  for (int m = m0; m < m1; ++m) {
    for (int n = n0; n < n1; ++n) {
      T *const restrict r = c + BN * (m-m0) + n-n0;
      for (int k = 0; k < SK; ++k) { /* transpose B-matrix */
        *r = FMA(a[SK*m+k], b[SN*k+n], *r);
      }
    }
  }

  { /* copy private tile back to global memory */
    global T *const restrict cwg = cmat + ci;
    for (int m = m0; m < m1; ++m) {
      for (int n = n0; n < n1; ++n) {
        ATOMIC_ADD_GLOBAL(&cwg[SM*n+m], c[BN*(m-m0)+n-n0]);
      }
    }
  }
}
