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
  GLOBAL const int *const restrict param_base = param_stack + gid * 3;
  /* indexes given by param_stack are one-based */
  const int ai = param_base[0] - 1, bi = param_base[1] - 1, ci = param_base[2] - 1;
  GLOBAL const T *const restrict awg = amat + ai, *const restrict bwg = bmat + bi;
  global T *const restrict cwg = cmat + ci;
  local T a[SM*SK];
  local T b[SK*SN];

  const int i = get_local_id(0);
  const int nbm = (SM + BM - 1) / BM;
  const int nbn = (SN + BN - 1) / BN;
  const int im = i / nbn, in = i - im * nbn;
  const int m0 = im * BM, m1 = min(m0 + BM, SM);
  const int n0 = in * BN, n1 = min(n0 + BN, SN);

  for (int m = m0; m < m1; ++m) { /* transpose A-matrix */
    for (int k = 0; k < SK; ++k) a[SK*m+k] = awg[SM*k+m];
  }
  for (int k = 0; k < SK; ++k) { /* transpose B-matrix */
    for (int n = n0; n < n1; ++n) b[SK*n+k] = bwg[SN*k+n];
  }
  barrier(CLK_LOCAL_MEM_FENCE);
  for (int m = m0; m < m1; ++m) {
    for (int n = n0; n < n1; ++n) {
      T r = 0;
      for (int k = 0; k < SK; ++k) {
        r = FMA(a[SK*m+k], b[SK*n+k], r);
      }
      ATOMIC_ADD_GLOBAL(&cwg[SM*n+m], r);
    }
  }
}
