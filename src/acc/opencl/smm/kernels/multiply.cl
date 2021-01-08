/*------------------------------------------------------------------------------------------------*
 * Copyright (C) by the DBCSR developers group - All rights reserved                              *
 * This file is part of the DBCSR library.                                                        *
 *                                                                                                *
 * For information on the license, see the LICENSE file.                                          *
 * For further information please visit https://dbcsr.cp2k.org                                    *
 * SPDX-License-Identifier: GPL-2.0+                                                              *
 *------------------------------------------------------------------------------------------------*/

/* number of M-blocks */
#define NBM ((SM + BM - 1) / BM)
/* number of N-blocks */
#define NBN ((SN + BN - 1) / BN)
/* number of blocks (tiles) */
#define NTL (NBM * NBN)
/* size of workgroup (WG) */
#define SWG (NTL * BS)


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
  const int gid = get_group_id(0), idx = get_local_id(0) / BS;
  GLOBAL const int *const restrict params = param_stack + (3 * BS) * gid;
  /* indexes given by param_stack are one-based */
  int a0 = params[0] - 1, b0 = params[1] - 1, c0 = params[2] - 1;
  global T *restrict cwg = cmat + c0;

  local T a[SM*SK];
#if (1 != BM) || (SN != BN)
  local T b[SK*SN];
# if (1 < BS)
  T c[BM*BN] = { 0 };
# endif
#else
  T b[SK];
# if (1 < BS)
  T c[SM] = { 0 };
# endif
#endif

  /* intra-kernel mini-batch of SMMs */
  for (int i = 0; i < BS; ++i) {
#if (1 != BM) || (SN != BN)
    const int im = idx / NBN;
    const int m0 = im * BM, m1 = min(m0 + BM, SM);
    const int n0 = (idx - im * NBN) * BN;
    const int n1 = min(n0 + BN, SN);
#else
    const int m0 = idx * BM, m1 = min(m0 + BM, SM);
    const int n = idx;
#endif
    int a1, b1, c1;

    if (i < (BS - 1)) {
      a1 = params[3*i+3] - 1;
      b1 = params[3*i+4] - 1;
      c1 = params[3*i+5] - 1;
    }
    else {
      a1 = b1 = c1 = -1;
    }

    if (a0 != a1 || 0 == i) { /* transpose A-matrix into local buffer */
      GLOBAL const T *const restrict awg = amat + a0;
      for (int m = m0; m < m1; ++m) {
        for (int k = 0; k < SK; ++k) a[SK*m+k] = awg[SM*k+m];
      }
      /* next iteration */
      a0 = a1;
    }

    if (b0 != b1 || 0 == i) { /* copy B-matrix into local buffer */
      GLOBAL const T *const restrict bwg = bmat + b0;
      for (int k = 0; k < SK; ++k) {
#if (1 != BM) || (SN != BN)
        for (int n = n0; n < n1; ++n) b[SN*k+n] = bwg[SN*k+n];
#else
        b[k] = bwg[SN*k+n];
#endif
      }
      /* next iteration */
      b0 = b1;
    }

    { /* calculate private result-tile */
      barrier(CLK_LOCAL_MEM_FENCE);
#if (1 != BM) || (SN != BN)
      for (int m = m0; m < m1; ++m) for (int n = n0; n < n1; ++n) {
# if (1 < BS)
        T *const restrict r = c + BN * (m-m0) + n-n0;
        for (int k = 0; k < SK; ++k) *r = FMA(a[SK*m+k], b[SN*k+n], *r);
# else
        T r = 0;
        for (int k = 0; k < SK; ++k) r = FMA(a[SK*m+k], b[SN*k+n], r);
        ATOMIC_ADD_GLOBAL(&cwg[SM*n+m], r);
# endif
      }
#else
      for (int m = 0; m < SM; ++m) {
# if (1 < BS)
        T *const restrict r = c + m;
        for (int k = 0; k < SK; ++k) *r = FMA(a[SK*m+k], b[k], *r);
# else
        T r = 0;
        for (int k = 0; k < SK; ++k) r = FMA(a[SK*m+k], b[k], r);
        ATOMIC_ADD_GLOBAL(&cwg[SM*n+m], r);
# endif
      }
#endif
    }

#if (1 < BS)
    if (c0 != c1) { /* copy private tile to global memory */
# if (1 != BM) || (SN != BN)
      for (int m = m0; m < m1; ++m) for (int n = n0; n < n1; ++n) {
        ATOMIC_ADD_GLOBAL(&cwg[SM*n+m], c[BN*(m-m0)+n-n0]);
      }
# else
      for (int m = 0; m < SM; ++m) {
        ATOMIC_ADD_GLOBAL(&cwg[SM*n+m], c[m]);
      }
# endif
      /* next iteration */
      cwg = cmat + c1;
      c0 = c1;
    }
#endif
  }
}
