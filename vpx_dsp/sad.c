/*
 *  Copyright (c) 2015 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>

#include "./vpx_config.h"
#include "./vpx_dsp_rtcd.h"

#include "vpx/vpx_integer.h"
#include "vpx_ports/mem.h"

/* Sum the difference between every corresponding element of the buffers. */
static INLINE unsigned int sad(const uint8_t *a, int a_stride,
                               const uint8_t *b, int b_stride,
                               int width, int height) {
  int y, x;
  unsigned int sad = 0;

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += abs(a[x] - b[x]);

    a += a_stride;
    b += b_stride;
  }
  return sad;
}

#define sadMxN(m, n) \
unsigned int vpx_sad##m##x##n##_c(const uint8_t *src, int src_stride, \
                                  const uint8_t *ref, int ref_stride) { \
  return sad(src, src_stride, ref, ref_stride, m, n); \
} \
unsigned int vpx_sad##m##x##n##_avg_c(const uint8_t *src, int src_stride, \
                                      const uint8_t *ref, int ref_stride, \
                                      const uint8_t *second_pred) { \
  uint8_t comp_pred[m * n]; \
  vpx_comp_avg_pred_c(comp_pred, second_pred, m, n, ref, ref_stride); \
  return sad(src, src_stride, comp_pred, m, m, n); \
}

// depending on call sites, pass **ref_array to avoid & in subsequent call and
// de-dup with 4D below.
#define sadMxNxK(m, n, k) \
void vpx_sad##m##x##n##x##k##_c(const uint8_t *src, int src_stride, \
                                const uint8_t *ref_array, int ref_stride, \
                                uint32_t *sad_array) { \
  int i; \
  for (i = 0; i < k; ++i) \
    sad_array[i] = vpx_sad##m##x##n##_c(src, src_stride, &ref_array[i], ref_stride); \
}

// This appears to be equivalent to the above when k == 4 and refs is const
#define sadMxNx4D(m, n) \
void vpx_sad##m##x##n##x4d_c(const uint8_t *src, int src_stride, \
                             const uint8_t *const ref_array[], int ref_stride, \
                             uint32_t *sad_array) { \
  int i; \
  for (i = 0; i < 4; ++i) \
    sad_array[i] = vpx_sad##m##x##n##_c(src, src_stride, ref_array[i], ref_stride); \
}

#if CONFIG_VP10 && CONFIG_EXT_PARTITION
// 128x128
sadMxN(128, 128)
sadMxNxK(128, 128, 3)
sadMxNxK(128, 128, 8)
sadMxNx4D(128, 128)

// 128x64
sadMxN(128, 64)
sadMxNx4D(128, 64)

// 64x128
sadMxN(64, 128)
sadMxNx4D(64, 128)
#endif  // CONFIG_VP10 && CONFIG_EXT_PARTITION

// 64x64
sadMxN(64, 64)
sadMxNxK(64, 64, 3)
sadMxNxK(64, 64, 8)
sadMxNx4D(64, 64)

// 64x32
sadMxN(64, 32)
sadMxNx4D(64, 32)

// 32x64
sadMxN(32, 64)
sadMxNx4D(32, 64)

// 32x32
sadMxN(32, 32)
sadMxNxK(32, 32, 3)
sadMxNxK(32, 32, 8)
sadMxNx4D(32, 32)

// 32x16
sadMxN(32, 16)
sadMxNx4D(32, 16)

// 16x32
sadMxN(16, 32)
sadMxNx4D(16, 32)

// 16x16
sadMxN(16, 16)
sadMxNxK(16, 16, 3)
sadMxNxK(16, 16, 8)
sadMxNx4D(16, 16)

// 16x8
sadMxN(16, 8)
sadMxNxK(16, 8, 3)
sadMxNxK(16, 8, 8)
sadMxNx4D(16, 8)

// 8x16
sadMxN(8, 16)
sadMxNxK(8, 16, 3)
sadMxNxK(8, 16, 8)
sadMxNx4D(8, 16)

// 8x8
sadMxN(8, 8)
sadMxNxK(8, 8, 3)
sadMxNxK(8, 8, 8)
sadMxNx4D(8, 8)

// 8x4
sadMxN(8, 4)
sadMxNxK(8, 4, 8)
sadMxNx4D(8, 4)

// 4x8
sadMxN(4, 8)
sadMxNxK(4, 8, 8)
sadMxNx4D(4, 8)

// 4x4
sadMxN(4, 4)
sadMxNxK(4, 4, 3)
sadMxNxK(4, 4, 8)
sadMxNx4D(4, 4)

#if CONFIG_VPX_HIGHBITDEPTH
static INLINE unsigned int highbd_sad(const uint8_t *a8, int a_stride,
                                      const uint8_t *b8, int b_stride,
                                      int width, int height) {
  int y, x;
  unsigned int sad = 0;
  const uint16_t *a = CONVERT_TO_SHORTPTR(a8);
  const uint16_t *b = CONVERT_TO_SHORTPTR(b8);
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += abs(a[x] - b[x]);

    a += a_stride;
    b += b_stride;
  }
  return sad;
}

static INLINE unsigned int highbd_sadb(const uint8_t *a8, int a_stride,
                                       const uint16_t *b, int b_stride,
                                       int width, int height) {
  int y, x;
  unsigned int sad = 0;
  const uint16_t *a = CONVERT_TO_SHORTPTR(a8);
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += abs(a[x] - b[x]);

    a += a_stride;
    b += b_stride;
  }
  return sad;
}

#define highbd_sadMxN(m, n) \
unsigned int vpx_highbd_sad##m##x##n##_c(const uint8_t *src, int src_stride, \
                                         const uint8_t *ref, int ref_stride) { \
  return highbd_sad(src, src_stride, ref, ref_stride, m, n); \
} \
unsigned int vpx_highbd_sad##m##x##n##_avg_c(const uint8_t *src, \
                                             int src_stride, \
                                             const uint8_t *ref, \
                                             int ref_stride, \
                                             const uint8_t *second_pred) { \
  uint16_t comp_pred[m * n]; \
  vpx_highbd_comp_avg_pred_c(comp_pred, second_pred, m, n, ref, ref_stride); \
  return highbd_sadb(src, src_stride, comp_pred, m, m, n); \
}

#define highbd_sadMxNxK(m, n, k) \
void vpx_highbd_sad##m##x##n##x##k##_c(const uint8_t *src, int src_stride, \
                                       const uint8_t *ref_array, int ref_stride, \
                                       uint32_t *sad_array) { \
  int i; \
  for (i = 0; i < k; ++i) { \
    sad_array[i] = vpx_highbd_sad##m##x##n##_c(src, src_stride, &ref_array[i], \
                                               ref_stride); \
  } \
}

#define highbd_sadMxNx4D(m, n) \
void vpx_highbd_sad##m##x##n##x4d_c(const uint8_t *src, int src_stride, \
                                    const uint8_t *const ref_array[], \
                                    int ref_stride, uint32_t *sad_array) { \
  int i; \
  for (i = 0; i < 4; ++i) { \
    sad_array[i] = vpx_highbd_sad##m##x##n##_c(src, src_stride, ref_array[i], \
                                               ref_stride); \
  } \
}

#if CONFIG_VP10 && CONFIG_EXT_PARTITION
// 128x128
highbd_sadMxN(128, 128)
highbd_sadMxNxK(128, 128, 3)
highbd_sadMxNxK(128, 128, 8)
highbd_sadMxNx4D(128, 128)

// 128x64
highbd_sadMxN(128, 64)
highbd_sadMxNx4D(128, 64)

// 64x128
highbd_sadMxN(64, 128)
highbd_sadMxNx4D(64, 128)
#endif  // CONFIG_VP10 && CONFIG_EXT_PARTITION

// 64x64
highbd_sadMxN(64, 64)
highbd_sadMxNxK(64, 64, 3)
highbd_sadMxNxK(64, 64, 8)
highbd_sadMxNx4D(64, 64)

// 64x32
highbd_sadMxN(64, 32)
highbd_sadMxNx4D(64, 32)

// 32x64
highbd_sadMxN(32, 64)
highbd_sadMxNx4D(32, 64)

// 32x32
highbd_sadMxN(32, 32)
highbd_sadMxNxK(32, 32, 3)
highbd_sadMxNxK(32, 32, 8)
highbd_sadMxNx4D(32, 32)

// 32x16
highbd_sadMxN(32, 16)
highbd_sadMxNx4D(32, 16)

// 16x32
highbd_sadMxN(16, 32)
highbd_sadMxNx4D(16, 32)

// 16x16
highbd_sadMxN(16, 16)
highbd_sadMxNxK(16, 16, 3)
highbd_sadMxNxK(16, 16, 8)
highbd_sadMxNx4D(16, 16)

// 16x8
highbd_sadMxN(16, 8)
highbd_sadMxNxK(16, 8, 3)
highbd_sadMxNxK(16, 8, 8)
highbd_sadMxNx4D(16, 8)

// 8x16
highbd_sadMxN(8, 16)
highbd_sadMxNxK(8, 16, 3)
highbd_sadMxNxK(8, 16, 8)
highbd_sadMxNx4D(8, 16)

// 8x8
highbd_sadMxN(8, 8)
highbd_sadMxNxK(8, 8, 3)
highbd_sadMxNxK(8, 8, 8)
highbd_sadMxNx4D(8, 8)

// 8x4
highbd_sadMxN(8, 4)
highbd_sadMxNxK(8, 4, 8)
highbd_sadMxNx4D(8, 4)

// 4x8
highbd_sadMxN(4, 8)
highbd_sadMxNxK(4, 8, 8)
highbd_sadMxNx4D(4, 8)

// 4x4
highbd_sadMxN(4, 4)
highbd_sadMxNxK(4, 4, 3)
highbd_sadMxNxK(4, 4, 8)
highbd_sadMxNx4D(4, 4)

#endif  // CONFIG_VPX_HIGHBITDEPTH

#if CONFIG_VP10 && CONFIG_EXT_INTER
static INLINE unsigned int masked_sad(const uint8_t *a, int a_stride,
                                      const uint8_t *b, int b_stride,
                                      const uint8_t *m, int m_stride,
                                      int width, int height) {
  int y, x;
  unsigned int sad = 0;

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += m[x] * abs(a[x] - b[x]);

    a += a_stride;
    b += b_stride;
    m += m_stride;
  }
  sad = (sad + 31) >> 6;

  return sad;
}

#define MASKSADMxN(m, n) \
unsigned int vpx_masked_sad##m##x##n##_c(const uint8_t *src, int src_stride, \
                                         const uint8_t *ref, int ref_stride, \
                                         const uint8_t *msk, int msk_stride) { \
  return masked_sad(src, src_stride, ref, ref_stride, msk, msk_stride, m, n); \
}

#if CONFIG_EXT_PARTITION
MASKSADMxN(128, 128)
MASKSADMxN(128, 64)
MASKSADMxN(64, 128)
#endif  // CONFIG_EXT_PARTITION
MASKSADMxN(64, 64)
MASKSADMxN(64, 32)
MASKSADMxN(32, 64)
MASKSADMxN(32, 32)
MASKSADMxN(32, 16)
MASKSADMxN(16, 32)
MASKSADMxN(16, 16)
MASKSADMxN(16, 8)
MASKSADMxN(8, 16)
MASKSADMxN(8, 8)
MASKSADMxN(8, 4)
MASKSADMxN(4, 8)
MASKSADMxN(4, 4)

#if CONFIG_VPX_HIGHBITDEPTH
static INLINE unsigned int highbd_masked_sad(const uint8_t *a8, int a_stride,
                                             const uint8_t *b8, int b_stride,
                                             const uint8_t *m, int m_stride,
                                             int width, int height) {
  int y, x;
  unsigned int sad = 0;
  const uint16_t *a = CONVERT_TO_SHORTPTR(a8);
  const uint16_t *b = CONVERT_TO_SHORTPTR(b8);

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += m[x] * abs(a[x] - b[x]);

    a += a_stride;
    b += b_stride;
    m += m_stride;
  }
  sad = (sad + 31) >> 6;

  return sad;
}

#define HIGHBD_MASKSADMXN(m, n) \
unsigned int vpx_highbd_masked_sad##m##x##n##_c(const uint8_t *src, \
                                                int src_stride, \
                                                const uint8_t *ref, \
                                                int ref_stride, \
                                                const uint8_t *msk, \
                                                int msk_stride) { \
  return highbd_masked_sad(src, src_stride, ref, ref_stride, \
                           msk, msk_stride, m, n); \
}

#if CONFIG_EXT_PARTITION
HIGHBD_MASKSADMXN(128, 128)
HIGHBD_MASKSADMXN(128, 64)
HIGHBD_MASKSADMXN(64, 128)
#endif  // CONFIG_EXT_PARTITION
HIGHBD_MASKSADMXN(64, 64)
HIGHBD_MASKSADMXN(64, 32)
HIGHBD_MASKSADMXN(32, 64)
HIGHBD_MASKSADMXN(32, 32)
HIGHBD_MASKSADMXN(32, 16)
HIGHBD_MASKSADMXN(16, 32)
HIGHBD_MASKSADMXN(16, 16)
HIGHBD_MASKSADMXN(16, 8)
HIGHBD_MASKSADMXN(8, 16)
HIGHBD_MASKSADMXN(8, 8)
HIGHBD_MASKSADMXN(8, 4)
HIGHBD_MASKSADMXN(4, 8)
HIGHBD_MASKSADMXN(4, 4)
#endif  // CONFIG_VPX_HIGHBITDEPTH
#endif  // CONFIG_VP10 && CONFIG_EXT_INTER

#if CONFIG_VP10 && CONFIG_OBMC
// pre: predictor being evaluated
// wsrc: target weighted prediction (has been *4096 to keep precision)
// mask: 2d weights (scaled by 4096)
static INLINE unsigned int obmc_sad(const uint8_t *pre, int pre_stride,
                                    const int32_t *wsrc,
                                    const int32_t *mask,
                                    int width, int height) {
  int y, x;
  unsigned int sad = 0;

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += ROUND_POWER_OF_TWO(abs(wsrc[x] - pre[x] * mask[x]), 12);

    pre += pre_stride;
    wsrc += width;
    mask += width;
  }

  return sad;
}

#define OBMCSADMxN(m, n)                                                      \
unsigned int vpx_obmc_sad##m##x##n##_c(const uint8_t *ref, int ref_stride,    \
                                       const int32_t *wsrc,                   \
                                       const int32_t *mask) {                 \
  return obmc_sad(ref, ref_stride, wsrc, mask, m, n);                         \
}

#if CONFIG_EXT_PARTITION
OBMCSADMxN(128, 128)
OBMCSADMxN(128, 64)
OBMCSADMxN(64, 128)
#endif  // CONFIG_EXT_PARTITION
OBMCSADMxN(64, 64)
OBMCSADMxN(64, 32)
OBMCSADMxN(32, 64)
OBMCSADMxN(32, 32)
OBMCSADMxN(32, 16)
OBMCSADMxN(16, 32)
OBMCSADMxN(16, 16)
OBMCSADMxN(16, 8)
OBMCSADMxN(8, 16)
OBMCSADMxN(8, 8)
OBMCSADMxN(8, 4)
OBMCSADMxN(4, 8)
OBMCSADMxN(4, 4)

#if CONFIG_VPX_HIGHBITDEPTH
static INLINE unsigned int highbd_obmc_sad(const uint8_t *pre8, int pre_stride,
                                           const int32_t *wsrc,
                                           const int32_t *mask,
                                           int width, int height) {
  int y, x;
  unsigned int sad = 0;
  const uint16_t *pre = CONVERT_TO_SHORTPTR(pre8);

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += ROUND_POWER_OF_TWO(abs(wsrc[x] - pre[x] * mask[x]), 12);

    pre += pre_stride;
    wsrc += width;
    mask += width;
  }

  return sad;
}

#define HIGHBD_OBMCSADMXN(m, n)                                               \
unsigned int vpx_highbd_obmc_sad##m##x##n##_c(const uint8_t *ref,             \
                                              int ref_stride,                 \
                                              const int32_t *wsrc,            \
                                              const int32_t *mask) {          \
  return highbd_obmc_sad(ref, ref_stride, wsrc, mask, m, n);                  \
}

#if CONFIG_EXT_PARTITION
HIGHBD_OBMCSADMXN(128, 128)
HIGHBD_OBMCSADMXN(128, 64)
HIGHBD_OBMCSADMXN(64, 128)
#endif  // CONFIG_EXT_PARTITION
HIGHBD_OBMCSADMXN(64, 64)
HIGHBD_OBMCSADMXN(64, 32)
HIGHBD_OBMCSADMXN(32, 64)
HIGHBD_OBMCSADMXN(32, 32)
HIGHBD_OBMCSADMXN(32, 16)
HIGHBD_OBMCSADMXN(16, 32)
HIGHBD_OBMCSADMXN(16, 16)
HIGHBD_OBMCSADMXN(16, 8)
HIGHBD_OBMCSADMXN(8, 16)
HIGHBD_OBMCSADMXN(8, 8)
HIGHBD_OBMCSADMXN(8, 4)
HIGHBD_OBMCSADMXN(4, 8)
HIGHBD_OBMCSADMXN(4, 4)
#endif  // CONFIG_VPX_HIGHBITDEPTH
#endif  // CONFIG_VP10 && CONFIG_OBMC
