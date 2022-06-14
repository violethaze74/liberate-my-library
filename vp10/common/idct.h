/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP10_COMMON_IDCT_H_
#define VP10_COMMON_IDCT_H_

#include <assert.h>

#include "./vpx_config.h"
#include "vp10/common/blockd.h"
#include "vp10/common/common.h"
#include "vp10/common/enums.h"
#include "vpx_dsp/inv_txfm.h"
#include "vpx_dsp/txfm_common.h"
#include "vpx_ports/mem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct INV_TXFM_PARAM {
  TX_TYPE tx_type;
  TX_SIZE tx_size;
  int eob;
  int lossless;
#if CONFIG_VPX_HIGHBITDEPTH
  int bd;
#endif
} INV_TXFM_PARAM;

typedef void (*transform_1d)(const tran_low_t*, tran_low_t*);

typedef struct {
  transform_1d cols, rows;  // vertical and horizontal
} transform_2d;

#if CONFIG_VPX_HIGHBITDEPTH
typedef void (*highbd_transform_1d)(const tran_low_t*, tran_low_t*, int bd);

typedef struct {
  highbd_transform_1d cols, rows;  // vertical and horizontal
} highbd_transform_2d;
#endif  // CONFIG_VPX_HIGHBITDEPTH

#define MAX_TX_SCALE 1
int get_tx_scale(const MACROBLOCKD *const xd, const TX_TYPE tx_type,
                 const TX_SIZE tx_size);

void vp10_iwht4x4_add(const tran_low_t *input, uint8_t *dest, int stride,
                     int eob);
void vp10_idct4x4_add(const tran_low_t *input, uint8_t *dest, int stride,
                     int eob);
void vp10_idct8x8_add(const tran_low_t *input, uint8_t *dest, int stride,
                      int eob);
void vp10_idct16x16_add(const tran_low_t *input, uint8_t *dest, int stride,
                        int eob);
void vp10_idct32x32_add(const tran_low_t *input, uint8_t *dest, int stride,
                        int eob);

void vp10_inv_txfm_add_4x4(const tran_low_t *input, uint8_t *dest,
                           int stride, int eob, TX_TYPE tx_type, int lossless);
#if CONFIG_EXT_TX
void vp10_inv_txfm_add_8x4(const tran_low_t *input, uint8_t *dest,
                           int stride, int eob, TX_TYPE tx_type);
void vp10_inv_txfm_add_4x8(const tran_low_t *input, uint8_t *dest,
                           int stride, int eob, TX_TYPE tx_type);
#endif  // CONFIG_EXT_TX
void vp10_inv_txfm_add_8x8(const tran_low_t *input, uint8_t *dest,
                           int stride, int eob, TX_TYPE tx_type);
void vp10_inv_txfm_add_16x16(const tran_low_t *input, uint8_t *dest,
                             int stride, int eob, TX_TYPE tx_type);
void vp10_inv_txfm_add_32x32(const tran_low_t *input, uint8_t *dest,
                             int stride, int eob, TX_TYPE tx_type);
void inv_txfm_add(const tran_low_t *input, uint8_t *dest, int stride,
                  INV_TXFM_PARAM *inv_txfm_param);
#if CONFIG_VPX_HIGHBITDEPTH
void vp10_highbd_iwht4x4_add(const tran_low_t *input, uint8_t *dest, int stride,
                            int eob, int bd);
void vp10_highbd_idct4x4_add(const tran_low_t *input, uint8_t *dest, int stride,
                            int eob, int bd);
void vp10_highbd_idct8x8_add(const tran_low_t *input, uint8_t *dest, int stride,
                            int eob, int bd);
void vp10_highbd_idct16x16_add(const tran_low_t *input, uint8_t *dest,
                              int stride, int eob, int bd);
void vp10_highbd_idct32x32_add(const tran_low_t *input, uint8_t *dest,
                              int stride, int eob, int bd);
void vp10_highbd_inv_txfm_add_4x4(const tran_low_t *input, uint8_t *dest,
                                  int stride, int eob, int bd, TX_TYPE tx_type,
                                  int lossless);
#if CONFIG_EXT_TX
void vp10_highbd_inv_txfm_add_4x8(const tran_low_t *input, uint8_t *dest,
                                  int stride, int eob, int bd, TX_TYPE tx_type);
void vp10_highbd_inv_txfm_add_8x4(const tran_low_t *input, uint8_t *dest,
                                  int stride, int eob, int bd, TX_TYPE tx_type);
#endif  // CONFIG_EXT_TX
void vp10_highbd_inv_txfm_add_8x8(const tran_low_t *input, uint8_t *dest,
                                  int stride, int eob, int bd, TX_TYPE tx_type);
void vp10_highbd_inv_txfm_add_16x16(const tran_low_t *input, uint8_t *dest,
                                    int stride, int eob, int bd,
                                    TX_TYPE tx_type);
void vp10_highbd_inv_txfm_add_32x32(const tran_low_t *input, uint8_t *dest,
                                    int stride, int eob, int bd,
                                    TX_TYPE tx_type);
void highbd_inv_txfm_add(const tran_low_t *input, uint8_t *dest, int stride,
                         INV_TXFM_PARAM *inv_txfm_param);
#endif  // CONFIG_VPX_HIGHBITDEPTH
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP10_COMMON_IDCT_H_
