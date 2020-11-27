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


kernel void FN(global const int3 *restrict param_stack,
  global const T *restrict amat, global const T *restrict bmat, global T *restrict cmat)
{
  /* indexes given by param_stack are one-based */
  const int3 idx = param_stack[get_group_id(0)] - 1;
  global const T *const restrict a = amat + idx.s0;
  global const T *const restrict b = bmat + idx.s1;
  global T *const restrict c = cmat + idx.s2;
  private T buf[SK];

  const int index = get_local_id(0);
  switch (get_local_size(0)) {
    case SN: {
      const int n = index;
      for (int k = 0; k < SK; ++k) buf[k] = b[SN*k+n];
      for (int m = 0; m < SM; ++m) {
        T r = 0;
        for (int k = 0; k < SK; ++k) r += a[SM*k+m] * buf[k];
        add_atomic(c + SM * n + m, r);
      }
    } break;
    default: if (index < SN) {
    }
  }
}
