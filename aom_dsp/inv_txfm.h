/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_DSP_INV_TXFM_H_
#define AOM_DSP_INV_TXFM_H_

#include <assert.h>

#include "./aom_config.h"
#include "aom_dsp/txfm_common.h"
#include "aom_ports/mem.h"

#ifdef __cplusplus
extern "C" {
#endif

static INLINE tran_high_t check_range(tran_high_t input) {
#if CONFIG_COEFFICIENT_RANGE_CHECKING
  // For valid AV1 input streams, intermediate stage coefficients should always
  // stay within the range of a signed 16 bit integer. Coefficients can go out
  // of this range for invalid/corrupt AV1 streams. However, strictly checking
  // this range for every intermediate coefficient can burdensome for a decoder,
  // therefore the following assertion is only enabled when configured with
  // --enable-coefficient-range-checking.
  assert(INT16_MIN <= input);
  assert(input <= INT16_MAX);
#endif  // CONFIG_COEFFICIENT_RANGE_CHECKING
  return input;
}

static INLINE tran_high_t dct_const_round_shift(tran_high_t input) {
  tran_high_t rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  return rv;
}

#if CONFIG_AOM_HIGHBITDEPTH
static INLINE tran_high_t highbd_check_range(tran_high_t input, int bd) {
#if CONFIG_COEFFICIENT_RANGE_CHECKING
  // For valid highbitdepth AV1 streams, intermediate stage coefficients will
  // stay within the ranges:
  // - 8 bit: signed 16 bit integer
  // - 10 bit: signed 18 bit integer
  // - 12 bit: signed 20 bit integer
  const int32_t int_max = (1 << (7 + bd)) - 1;
  const int32_t int_min = -int_max - 1;
  assert(int_min <= input);
  assert(input <= int_max);
  (void)int_min;
#endif  // CONFIG_COEFFICIENT_RANGE_CHECKING
  (void)bd;
  return input;
}

static INLINE tran_high_t highbd_dct_const_round_shift(tran_high_t input) {
  tran_high_t rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  return rv;
}
#endif  // CONFIG_AOM_HIGHBITDEPTH

#if CONFIG_EMULATE_HARDWARE
// When CONFIG_EMULATE_HARDWARE is 1 the transform performs a
// non-normative method to handle overflows. A stream that causes
// overflows  in the inverse transform is considered invalid,
// and a hardware implementer is free to choose any reasonable
// method to handle overflows. However to aid in hardware
// verification they can use a specific implementation of the
// WRAPLOW() macro below that is identical to their intended
// hardware implementation (and also use configure options to trigger
// the C-implementation of the transform).
//
// The particular WRAPLOW implementation below performs strict
// overflow wrapping to match common hardware implementations.
// bd of 8 uses trans_low with 16bits, need to remove 16bits
// bd of 10 uses trans_low with 18bits, need to remove 14bits
// bd of 12 uses trans_low with 20bits, need to remove 12bits
// bd of x uses trans_low with 8+x bits, need to remove 24-x bits

#define WRAPLOW(x) ((((int32_t)check_range(x)) << 16) >> 16)
#if CONFIG_AOM_HIGHBITDEPTH
#define HIGHBD_WRAPLOW(x, bd) \
  ((((int32_t)highbd_check_range((x), bd)) << (24 - bd)) >> (24 - bd))
#endif  // CONFIG_AOM_HIGHBITDEPTH

#else  // CONFIG_EMULATE_HARDWARE

#define WRAPLOW(x) ((int32_t)check_range(x))
#if CONFIG_AOM_HIGHBITDEPTH
#define HIGHBD_WRAPLOW(x, bd) ((int32_t)highbd_check_range((x), bd))
#endif  // CONFIG_AOM_HIGHBITDEPTH
#endif  // CONFIG_EMULATE_HARDWARE

void aom_idct4_c(const tran_low_t *input, tran_low_t *output);
void aom_idct8_c(const tran_low_t *input, tran_low_t *output);
void aom_idct16_c(const tran_low_t *input, tran_low_t *output);
void aom_idct32_c(const tran_low_t *input, tran_low_t *output);
void aom_iadst4_c(const tran_low_t *input, tran_low_t *output);
void aom_iadst8_c(const tran_low_t *input, tran_low_t *output);
void aom_iadst16_c(const tran_low_t *input, tran_low_t *output);

#if CONFIG_AOM_HIGHBITDEPTH
void aom_highbd_idct4_c(const tran_low_t *input, tran_low_t *output, int bd);
void aom_highbd_idct8_c(const tran_low_t *input, tran_low_t *output, int bd);
void aom_highbd_idct16_c(const tran_low_t *input, tran_low_t *output, int bd);
void aom_highbd_idct32_c(const tran_low_t *input, tran_low_t *output, int bd);

void aom_highbd_iadst4_c(const tran_low_t *input, tran_low_t *output, int bd);
void aom_highbd_iadst8_c(const tran_low_t *input, tran_low_t *output, int bd);
void aom_highbd_iadst16_c(const tran_low_t *input, tran_low_t *output, int bd);

static INLINE uint16_t highbd_clip_pixel_add(uint16_t dest, tran_high_t trans,
                                             int bd) {
  trans = HIGHBD_WRAPLOW(trans, bd);
  return clip_pixel_highbd(dest + (int)trans, bd);
}
#endif

static INLINE uint8_t clip_pixel_add(uint8_t dest, tran_high_t trans) {
  trans = WRAPLOW(trans);
  return clip_pixel(dest + (int)trans);
}
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_DSP_INV_TXFM_H_
