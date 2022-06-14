/*
 *  Copyright (c) 2015 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_DSP_VPX_DSP_COMMON_H_
#define VPX_DSP_VPX_DSP_COMMON_H_

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"
#include "vpx_ports/mem.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_SB_SIZE
# if CONFIG_VP10 && CONFIG_EXT_PARTITION
#   define MAX_SB_SIZE 128
# else
#   define MAX_SB_SIZE 64
# endif  // CONFIG_VP10 && CONFIG_EXT_PARTITION
#endif  // ndef MAX_SB_SIZE

#define VPXMIN(x, y) (((x) < (y)) ? (x) : (y))
#define VPXMAX(x, y) (((x) > (y)) ? (x) : (y))

#define IMPLIES(a, b)  (!(a) || (b))  //  Logical 'a implies b' (or 'a -> b')

#define IS_POWER_OF_TWO(x)  (((x) & ((x) - 1)) == 0)

// These can be used to give a hint about branch outcomes.
// This can have an effect, even if your target processor has a
// good branch predictor, as these hints can affect basic block
// ordering by the compiler.
#ifdef __GNUC__
# define LIKELY(v)    __builtin_expect(v, 1)
# define UNLIKELY(v)  __builtin_expect(v, 0)
#else
# define LIKELY(v)    (v)
# define UNLIKELY(v)  (v)
#endif

#define VPX_SWAP(type, a, b) \
  do {                       \
    type c = (b);            \
    b = a;                   \
    a = c;                   \
  } while (0)

#if CONFIG_VPX_HIGHBITDEPTH
// Note:
// tran_low_t  is the datatype used for final transform coefficients.
// tran_high_t is the datatype used for intermediate transform stages.
typedef int64_t tran_high_t;
typedef int32_t tran_low_t;
#else
// Note:
// tran_low_t  is the datatype used for final transform coefficients.
// tran_high_t is the datatype used for intermediate transform stages.
typedef int32_t tran_high_t;
typedef int16_t tran_low_t;
#endif  // CONFIG_VPX_HIGHBITDEPTH

static INLINE uint8_t clip_pixel(int val) {
  return (val > 255) ? 255 : (val < 0) ? 0 : val;
}

static INLINE int clamp(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

static INLINE double fclamp(double value, double low, double high) {
  return value < low ? low : (value > high ? high : value);
}

#if CONFIG_VPX_HIGHBITDEPTH
static INLINE uint16_t clip_pixel_highbd(int val, int bd) {
  switch (bd) {
    case 8:
    default:
      return (uint16_t)clamp(val, 0, 255);
    case 10:
      return (uint16_t)clamp(val, 0, 1023);
    case 12:
      return (uint16_t)clamp(val, 0, 4095);
  }
}
#endif  // CONFIG_VPX_HIGHBITDEPTH

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_DSP_VPX_DSP_COMMON_H_
