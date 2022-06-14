/*
 *  Copyright (c) 2015 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>

#include "./aom_dsp_rtcd.h"

uint64_t aom_sum_squares_2d_i16_c(const int16_t *src, int src_stride,
                                  int size) {
  int r, c;
  uint64_t ss = 0;

  for (r = 0; r < size; r++) {
    for (c = 0; c < size; c++) {
      const int16_t v = src[c];
      ss += v * v;
    }
    src += src_stride;
  }

  return ss;
}

uint64_t aom_sum_squares_i16_c(const int16_t *src, uint32_t n) {
  uint64_t ss = 0;
  do {
    const int16_t v = *src++;
    ss += v * v;
  } while (--n);

  return ss;
}
