/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_ENCODER_VP9_ENCODEMB_H_
#define VP9_ENCODER_VP9_ENCODEMB_H_

#include "./vpx_config.h"
#include "vp9/encoder/vp9_block.h"
#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/common/vp9_onyxc_int.h"

struct optimize_ctx {
  ENTROPY_CONTEXT ta[MAX_MB_PLANE][16];
  ENTROPY_CONTEXT tl[MAX_MB_PLANE][16];
};

struct encode_b_args {
  MACROBLOCK *x;
  struct optimize_ctx *ctx;
  unsigned char *skip_coeff;
};

void vp9_encode_sb(MACROBLOCK *x, BLOCK_SIZE bsize);
void vp9_encode_sby(MACROBLOCK *x, BLOCK_SIZE bsize);

void vp9_xform_quant(int plane, int block, BLOCK_SIZE plane_bsize,
                     TX_SIZE tx_size, void *arg);

void vp9_subtract_sby(MACROBLOCK *x, BLOCK_SIZE bsize);
void vp9_subtract_sbuv(MACROBLOCK *x, BLOCK_SIZE bsize);
void vp9_subtract_sb(MACROBLOCK *x, BLOCK_SIZE bsize);

void vp9_encode_block_intra(int plane, int block, BLOCK_SIZE plane_bsize,
                            TX_SIZE tx_size, void *arg);

void vp9_encode_intra_block_y(MACROBLOCK *x, BLOCK_SIZE bsize);
void vp9_encode_intra_block_uv(MACROBLOCK *x, BLOCK_SIZE bsize);

int vp9_encode_intra(MACROBLOCK *x, int use_16x16_pred);
void vp9_setup_interp_filters(MACROBLOCKD *xd,
                              INTERPOLATION_TYPE mcomp_filter_type,
                              VP9_COMMON *cm);
#endif  // VP9_ENCODER_VP9_ENCODEMB_H_
