/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_entropy.h"

#include "vp9/encoder/vp9_boolhuff.h"
#include "vp9/encoder/vp9_treewriter.h"

#define vp9_cost_upd256  ((int)(vp9_cost_one(upd) - vp9_cost_zero(upd)))

static int update_bits[255];

static int count_uniform(int v, int n) {
  int l = get_unsigned_bits(n);
  int m;
  if (l == 0) return 0;
  m = (1 << l) - n;
  if (v < m)
    return l - 1;
  else
    return l;
}

static int split_index(int i, int n, int modulus) {
  int max1 = (n - 1 - modulus / 2) / modulus + 1;
  if (i % modulus == modulus / 2)
    i = i / modulus;
  else
    i = max1 + i - (i + modulus - modulus / 2) / modulus;
  return i;
}

static int recenter_nonneg(int v, int m) {
  if (v > (m << 1))
    return v;
  else if (v >= m)
    return ((v - m) << 1);
  else
    return ((m - v) << 1) - 1;
}

static int remap_prob(int v, int m) {
  int i;
  static const int map_table[MAX_PROB - 1] = {
    // generated by:
    //   map_table[j] = split_index(j, MAX_PROB - 1, MODULUS_PARAM);
     20,  21,  22,  23,  24,  25,   0,  26,  27,  28,  29,  30,  31,  32,  33,
     34,  35,  36,  37,   1,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,   2,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,
      3,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,   4,  74,
     75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,   5,  86,  87,  88,
     89,  90,  91,  92,  93,  94,  95,  96,  97,   6,  98,  99, 100, 101, 102,
    103, 104, 105, 106, 107, 108, 109,   7, 110, 111, 112, 113, 114, 115, 116,
    117, 118, 119, 120, 121,   8, 122, 123, 124, 125, 126, 127, 128, 129, 130,
    131, 132, 133,   9, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
    145,  10, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157,  11,
    158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,  12, 170, 171,
    172, 173, 174, 175, 176, 177, 178, 179, 180, 181,  13, 182, 183, 184, 185,
    186, 187, 188, 189, 190, 191, 192, 193,  14, 194, 195, 196, 197, 198, 199,
    200, 201, 202, 203, 204, 205,  15, 206, 207, 208, 209, 210, 211, 212, 213,
    214, 215, 216, 217,  16, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
    228, 229,  17, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
     18, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253,  19,
  };
  v--;
  m--;
  if ((m << 1) <= MAX_PROB)
    i = recenter_nonneg(v, m) - 1;
  else
    i = recenter_nonneg(MAX_PROB - 1 - v, MAX_PROB - 1 - m) - 1;

  i = map_table[i];
  return i;
}

static int count_term_subexp(int word, int k, int num_syms) {
  int count = 0;
  int i = 0;
  int mk = 0;
  while (1) {
    int b = (i ? k + i - 1 : k);
    int a = (1 << b);
    if (num_syms <= mk + 3 * a) {
      count += count_uniform(word - mk, num_syms - mk);
      break;
    } else {
      int t = (word >= mk + a);
      count++;
      if (t) {
        i = i + 1;
        mk += a;
      } else {
        count += b;
        break;
      }
    }
  }
  return count;
}

static int prob_diff_update_cost(vp9_prob newp, vp9_prob oldp) {
  int delp = remap_prob(newp, oldp);
  return update_bits[delp] * 256;
}

static void encode_uniform(vp9_writer *w, int v, int n) {
  int l = get_unsigned_bits(n);
  int m;
  if (l == 0)
    return;
  m = (1 << l) - n;
  if (v < m) {
    vp9_write_literal(w, v, l - 1);
  } else {
    vp9_write_literal(w, m + ((v - m) >> 1), l - 1);
    vp9_write_literal(w, (v - m) & 1, 1);
  }
}

static void encode_term_subexp(vp9_writer *w, int word, int k, int num_syms) {
  int i = 0;
  int mk = 0;
  while (1) {
    int b = (i ? k + i - 1 : k);
    int a = (1 << b);
    if (num_syms <= mk + 3 * a) {
      encode_uniform(w, word - mk, num_syms - mk);
      break;
    } else {
      int t = (word >= mk + a);
      vp9_write_literal(w, t, 1);
      if (t) {
        i = i + 1;
        mk += a;
      } else {
        vp9_write_literal(w, word - mk, b);
        break;
      }
    }
  }
}

void vp9_write_prob_diff_update(vp9_writer *w, vp9_prob newp, vp9_prob oldp) {
  const int delp = remap_prob(newp, oldp);
  encode_term_subexp(w, delp, SUBEXP_PARAM, 255);
}

void vp9_compute_update_table() {
  int i;
  for (i = 0; i < 254; i++)
    update_bits[i] = count_term_subexp(i, SUBEXP_PARAM, 255);
}

int vp9_prob_diff_update_savings_search(const unsigned int *ct,
                                        vp9_prob oldp, vp9_prob *bestp,
                                        vp9_prob upd) {
  const int old_b = cost_branch256(ct, oldp);
  int bestsavings = 0;
  vp9_prob newp, bestnewp = oldp;
  const int step = *bestp > oldp ? -1 : 1;

  for (newp = *bestp; newp != oldp; newp += step) {
    const int new_b = cost_branch256(ct, newp);
    const int update_b = prob_diff_update_cost(newp, oldp) + vp9_cost_upd256;
    const int savings = old_b - new_b - update_b;
    if (savings > bestsavings) {
      bestsavings = savings;
      bestnewp = newp;
    }
  }
  *bestp = bestnewp;
  return bestsavings;
}

int vp9_prob_diff_update_savings_search_model(const unsigned int *ct,
                                              const vp9_prob *oldp,
                                              vp9_prob *bestp,
                                              vp9_prob upd,
                                              int b, int r) {
  int i, old_b, new_b, update_b, savings, bestsavings, step;
  int newp;
  vp9_prob bestnewp, newplist[ENTROPY_NODES], oldplist[ENTROPY_NODES];
  vp9_model_to_full_probs(oldp, oldplist);
  vpx_memcpy(newplist, oldp, sizeof(vp9_prob) * UNCONSTRAINED_NODES);
  for (i = UNCONSTRAINED_NODES, old_b = 0; i < ENTROPY_NODES; ++i)
    old_b += cost_branch256(ct + 2 * i, oldplist[i]);
  old_b += cost_branch256(ct + 2 * PIVOT_NODE, oldplist[PIVOT_NODE]);

  bestsavings = 0;
  bestnewp = oldp[PIVOT_NODE];

  step = (*bestp > oldp[PIVOT_NODE] ? -1 : 1);

  for (newp = *bestp; newp != oldp[PIVOT_NODE]; newp += step) {
    if (newp < 1 || newp > 255)
      continue;
    newplist[PIVOT_NODE] = newp;
    vp9_model_to_full_probs(newplist, newplist);
    for (i = UNCONSTRAINED_NODES, new_b = 0; i < ENTROPY_NODES; ++i)
      new_b += cost_branch256(ct + 2 * i, newplist[i]);
    new_b += cost_branch256(ct + 2 * PIVOT_NODE, newplist[PIVOT_NODE]);
    update_b = prob_diff_update_cost(newp, oldp[PIVOT_NODE]) +
        vp9_cost_upd256;
    savings = old_b - new_b - update_b;
    if (savings > bestsavings) {
      bestsavings = savings;
      bestnewp = newp;
    }
  }
  *bestp = bestnewp;
  return bestsavings;
}

void vp9_cond_prob_diff_update(vp9_writer *w, vp9_prob *oldp,
                               const unsigned int ct[2]) {
  const vp9_prob upd = DIFF_UPDATE_PROB;
  vp9_prob newp = get_binary_prob(ct[0], ct[1]);
  const int savings = vp9_prob_diff_update_savings_search(ct, *oldp, &newp,
                                                          upd);
  assert(newp >= 1);
  if (savings > 0) {
    vp9_write(w, 1, upd);
    vp9_write_prob_diff_update(w, newp, *oldp);
    *oldp = newp;
  } else {
    vp9_write(w, 0, upd);
  }
}
