/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef VP9_COMMON_VP9_BLOCKD_H_
#define VP9_COMMON_VP9_BLOCKD_H_

#include "./vpx_config.h"

#include "vpx_ports/mem.h"
#include "vpx_scale/yv12config.h"

#include "vp9/common/vp9_common_data.h"
#include "vp9/common/vp9_filter.h"
#include "vp9/common/vp9_mv.h"
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_scale.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE_GROUPS 4
#define SKIP_CONTEXTS 3
#define INTER_MODE_CONTEXTS 7

#if CONFIG_SR_MODE
#define SR_CONTEXTS 3  // number of enalbed tx_size for sr mode

#define USE_POST_F 0  // 1: use post filters
#define SR_USE_MULTI_F 0  // 1: choose from multiple post filters

// SR_USFILTER_NUM_D: Number of 1D filters to choose in the post filter family
// SR_USFILTER_NUM: Number of combined 2D filters to choose
// If change this number, please change "idx_to_v","idx_to_h","hv_to_idx",
// and the prob model ("vp9_sr_usfilter_tree", "default_sr_usfilter_probs")
#define SR_USFILTER_NUM_D 4
#define SR_USFILTER_NUM (SR_USFILTER_NUM_D * SR_USFILTER_NUM_D)

#define SR_USFILTER_CONTEXTS 1
// SR_USFILTER_CONTEXTS: Depends on the post filters of upper and left blocks
#endif  // CONFIG_SR_MODE

#if CONFIG_COPY_MODE
#define COPY_MODE_CONTEXTS 5
#endif  // CONFIG_COPY_MODE

#if CONFIG_PALETTE
#define PALETTE_BUF_SIZE 16
#define PALETTE_MAX_SIZE 8
#define PALETTE_DELTA_BIT 0
#define PALETTE_COLOR_CONTEXTS 16
#endif  // CONFIG_PALETTE

/* Segment Feature Masks */
#define MAX_MV_REF_CANDIDATES 2

#define INTRA_INTER_CONTEXTS 4
#define COMP_INTER_CONTEXTS 5
#define REF_CONTEXTS 5

#if CONFIG_MULTI_REF

#define SINGLE_REFS 6
#define COMP_REFS 5

#else  // CONFIG_MULTI_REF
#define SINGLE_REFS 3
#define COMP_REFS 2
#endif  // CONFIG_MULTI_REF

#if CONFIG_NEW_QUANT
#define QUANT_PROFILES 3
#define Q_CTX_BASED_PROFILES 1

#if QUANT_PROFILES > 1

#define Q_THRESHOLD_MIN 0
#define Q_THRESHOLD_MAX 1000

static INLINE int switchable_dq_profile_used(int q_ctx, BLOCK_SIZE bsize) {
  return ((bsize >= BLOCK_32X32) * q_ctx);
}
#endif  // QUANT_PROFILES > 1
#endif  // CONFIG_NEW_QUANT

typedef enum {
  PLANE_TYPE_Y  = 0,
  PLANE_TYPE_UV = 1,
  PLANE_TYPES
} PLANE_TYPE;

#define MAX_MB_PLANE 3

typedef char ENTROPY_CONTEXT;

static INLINE int combine_entropy_contexts(ENTROPY_CONTEXT a,
                                           ENTROPY_CONTEXT b) {
  return (a != 0) + (b != 0);
}

typedef enum {
  KEY_FRAME = 0,
  INTER_FRAME = 1,
  FRAME_TYPES,
} FRAME_TYPE;

typedef enum {
  DC_PRED,         // Average of above and left pixels
  V_PRED,          // Vertical
  H_PRED,          // Horizontal
  D45_PRED,        // Directional 45  deg = round(arctan(1/1) * 180/pi)
  D135_PRED,       // Directional 135 deg = 180 - 45
  D117_PRED,       // Directional 117 deg = 180 - 63
  D153_PRED,       // Directional 153 deg = 180 - 27
  D207_PRED,       // Directional 207 deg = 180 + 27
  D63_PRED,        // Directional 63  deg = round(arctan(2/1) * 180/pi)
  TM_PRED,         // True-motion
#if CONFIG_INTRABC
  NEWDV,           // New displacement vector within the same frame buffer
#endif  // CONFIG_INTRABC
  NEARESTMV,
  NEARMV,
  ZEROMV,
  NEWMV,
#if CONFIG_NEW_INTER
  NEW2MV,
  NEAREST_NEARESTMV,
  NEAREST_NEARMV,
  NEAR_NEARESTMV,
  NEAREST_NEWMV,
  NEW_NEARESTMV,
  NEAR_NEWMV,
  NEW_NEARMV,
  ZERO_ZEROMV,
  NEW_NEWMV,
#endif  // CONFIG_NEW_INTER
  MB_MODE_COUNT
} PREDICTION_MODE;

#if CONFIG_COPY_MODE
typedef enum {
  NOREF,
  REF0,
  REF1,
  REF2,
  COPY_MODE_COUNT
} COPY_MODE;
#endif  // CONFIG_COPY_MODE

static INLINE int is_inter_mode(PREDICTION_MODE mode) {
#if CONFIG_NEW_INTER
  return mode >= NEARESTMV && mode <= NEW2MV;
#else
  return mode >= NEARESTMV && mode <= NEWMV;
#endif  // CONFIG_NEW_INTER
}

#if CONFIG_NEW_INTER
static INLINE int is_inter_compound_mode(PREDICTION_MODE mode) {
  return mode >= NEAREST_NEARESTMV && mode <= NEW_NEWMV;
}
#endif  // CONFIG_NEW_INTER

static INLINE int have_newmv_in_inter_mode(PREDICTION_MODE mode) {
#if CONFIG_NEW_INTER
  return (mode == NEWMV ||
          mode == NEW2MV ||
          mode == NEW_NEWMV ||
          mode == NEAREST_NEWMV ||
          mode == NEW_NEARESTMV ||
          mode == NEAR_NEWMV ||
          mode == NEW_NEARMV);
#else
  return (mode == NEWMV);
#endif  // CONFIG_NEW_INTER
}

#if CONFIG_INTRABC
static INLINE int is_intrabc_mode(PREDICTION_MODE mode) {
  return mode == NEWDV;
}
#endif  // CONFIG_INTRABC

#define INTRA_MODES (TM_PRED + 1)
#if CONFIG_NEW_INTER
#define INTER_MODES (1 + NEW2MV - NEARESTMV)
#else
#define INTER_MODES (1 + NEWMV - NEARESTMV)
#endif  // CONFIG_NEW_INTER

#define INTER_OFFSET(mode) ((mode) - NEARESTMV)

#if CONFIG_NEW_INTER

#define INTER_COMPOUND_MODES (1 + NEW_NEWMV - NEAREST_NEARESTMV)

#define INTER_COMPOUND_OFFSET(mode) ((mode) - NEAREST_NEARESTMV)

#endif  // CONFIG_NEW_INTER

#if CONFIG_TX64X64
#define MAXTXLEN 64
#else
#define MAXTXLEN 32
#endif

/* For keyframes, intra block modes are predicted by the (already decoded)
   modes for the Y blocks to the left and above us; for interframes, there
   is a single probability table. */

typedef struct {
  PREDICTION_MODE as_mode;
  int_mv as_mv[2];  // first, second inter predictor motion vectors
#if CONFIG_NEW_INTER
  int_mv ref_mv[2];
#endif  // CONFIG_NEW_INTER
} b_mode_info;

// Note that the rate-distortion optimization loop, bit-stream writer, and
// decoder implementation modules critically rely on the enum entry values
// specified herein. They should be refactored concurrently.
typedef enum {
  NONE = -1,
  INTRA_FRAME = 0,
  LAST_FRAME = 1,
#if CONFIG_MULTI_REF
  LAST2_FRAME = 2,
  LAST3_FRAME = 3,
  LAST4_FRAME = 4,
  GOLDEN_FRAME = 5,
  ALTREF_FRAME = 6,
#else  // CONFIG_MULTI_REF
  GOLDEN_FRAME = 2,
  ALTREF_FRAME = 3,
#endif  // CONFIG_MULTI_REF
  MAX_REF_FRAMES
} MV_REFERENCE_FRAME;

// This structure now relates to 8x8 block regions.
typedef struct {
  // Common for both INTER and INTRA blocks
  BLOCK_SIZE sb_type;
  PREDICTION_MODE mode;
#if CONFIG_FILTERINTRA
  int filterbit, uv_filterbit;
#endif
#if CONFIG_SR_MODE
  int sr;
  int us_filter_idx;
#endif  // CONFIG_SR_MODE
  TX_SIZE tx_size;
  int8_t skip;
  int8_t segment_id;
  int8_t seg_id_predicted;  // valid only when temporal_update is enabled

  // Only for INTRA blocks
  PREDICTION_MODE uv_mode;

  // Only for INTER blocks
  MV_REFERENCE_FRAME ref_frame[2];
  int_mv mv[2];
  int_mv ref_mvs[MAX_REF_FRAMES][MAX_MV_REF_CANDIDATES];
  uint8_t mode_context[MAX_REF_FRAMES];
  INTERP_FILTER interp_filter;

#if CONFIG_EXT_TX
  EXT_TX_TYPE ext_txfrm;
#endif
#if CONFIG_TX_SKIP
  int tx_skip[PLANE_TYPES];
  int tx_skip_shift;
#endif  // CONFIG_TX_SKIP
#if CONFIG_COPY_MODE
  COPY_MODE copy_mode;
  int inter_ref_count;
#endif  // CONFIG_COPY_MODE
#if CONFIG_INTERINTRA
  PREDICTION_MODE interintra_mode;
  PREDICTION_MODE interintra_uv_mode;
#if CONFIG_WEDGE_PARTITION
  int use_wedge_interintra;
  int interintra_wedge_index;
  int interintra_uv_wedge_index;
#endif  // CONFIG_WEDGE_PARTITION
#endif  // CONFIG_INTERINTRA
#if CONFIG_WEDGE_PARTITION
  int use_wedge_interinter;
  int interinter_wedge_index;
#endif  // CONFIG_WEDGE_PARTITION
#if CONFIG_PALETTE
  int palette_enabled[2];
  int palette_size[2];
  int palette_indexed_size;
  int palette_literal_size;
  int current_palette_size;
  int palette_delta_bitdepth;
  uint8_t palette_indexed_colors[PALETTE_MAX_SIZE];
  int8_t palette_color_delta[PALETTE_MAX_SIZE];
  uint8_t *palette_color_map;
  uint8_t *palette_uv_color_map;
#if CONFIG_VP9_HIGHBITDEPTH
  uint16_t palette_colors[3 * PALETTE_MAX_SIZE];
  uint16_t palette_literal_colors[PALETTE_MAX_SIZE];
#else
  uint8_t palette_colors[3 * PALETTE_MAX_SIZE];
  uint8_t palette_literal_colors[PALETTE_MAX_SIZE];
#endif  // CONFIG_VP9_HIGHBITDEPTH
#endif  // CONFIG_PALETTE
#if CONFIG_NEW_QUANT
  int dq_off_index;
  int send_dq_bit;
#endif  // CONFIG_NEW_QUANT
} MB_MODE_INFO;

typedef struct MODE_INFO {
  struct MODE_INFO *src_mi;
  MB_MODE_INFO mbmi;
#if CONFIG_FILTERINTRA
  int b_filter_info[4];
#endif
  b_mode_info bmi[4];
} MODE_INFO;

static INLINE PREDICTION_MODE get_y_mode(const MODE_INFO *mi, int block) {
  return mi->mbmi.sb_type < BLOCK_8X8 ? mi->bmi[block].as_mode
                                      : mi->mbmi.mode;
}

#if CONFIG_FILTERINTRA
static INLINE int is_filter_allowed(PREDICTION_MODE mode) {
#if CONFIG_INTRABC
  return !is_intrabc_mode(mode);
#else
  (void)mode;
  return 1;
#endif  // CONFIG_INTRABC
}

static INLINE int is_filter_enabled(TX_SIZE txsize) {
  return (txsize < TX_SIZES);
}
#endif

static INLINE int is_inter_block(const MB_MODE_INFO *mbmi) {
  return mbmi->ref_frame[0] > INTRA_FRAME;
}

static INLINE int has_second_ref(const MB_MODE_INFO *mbmi) {
  return mbmi->ref_frame[1] > INTRA_FRAME;
}

PREDICTION_MODE vp9_left_block_mode(const MODE_INFO *cur_mi,
                                    const MODE_INFO *left_mi, int b);

PREDICTION_MODE vp9_above_block_mode(const MODE_INFO *cur_mi,
                                     const MODE_INFO *above_mi, int b);

enum mv_precision {
  MV_PRECISION_Q3,
  MV_PRECISION_Q4
};

struct buf_2d {
  uint8_t *buf;
  uint8_t *buf0;
  int width;
  int height;
  int stride;
};

struct macroblockd_plane {
  tran_low_t *dqcoeff;
  PLANE_TYPE plane_type;
  int subsampling_x;
  int subsampling_y;
  struct buf_2d dst;
  struct buf_2d pre[2];
  const int16_t *dequant;
#if CONFIG_NEW_QUANT
  const dequant_val_type_nuq* dequant_val_nuq[QUANT_PROFILES];
#endif  // CONFIG_NEW_QUANT
#if CONFIG_TX_SKIP
  const int16_t *dequant_pxd;
#if CONFIG_NEW_QUANT
  const dequant_val_type_nuq* dequant_val_nuq_pxd[QUANT_PROFILES];
#endif  // CONFIG_NEW_QUANT
#endif  // CONFIG_TX_SKIP
  ENTROPY_CONTEXT *above_context;
  ENTROPY_CONTEXT *left_context;
#if CONFIG_PALETTE
  uint8_t *color_index_map;
#endif
};

#define BLOCK_OFFSET(x, i) ((x) + (i) * 16)

typedef struct RefBuffer {
  // TODO(dkovalev): idx is not really required and should be removed, now it
  // is used in vp9_onyxd_if.c
  int idx;
  YV12_BUFFER_CONFIG *buf;
  struct scale_factors sf;
} RefBuffer;

typedef struct macroblockd {
  struct macroblockd_plane plane[MAX_MB_PLANE];

  int mi_stride;

  MODE_INFO *mi;

  int up_available;
  int left_available;

  /* Distance of MB away from frame edges */
  int mb_to_left_edge;
  int mb_to_right_edge;
  int mb_to_top_edge;
  int mb_to_bottom_edge;

  /* pointers to reference frames */
  RefBuffer *block_refs[2];

  /* pointer to current frame */
  const YV12_BUFFER_CONFIG *cur_buf;

  // The size of mc_buf contains a x2 for each dimension because the image may
  // be no less than 2x smaller
  /* mc buffer */
  DECLARE_ALIGNED(16, uint8_t, mc_buf[(CODING_UNIT_SIZE + 16) * 2 *
                                      (CODING_UNIT_SIZE + 16) * 2]);
#if CONFIG_VP9_HIGHBITDEPTH
  /* Bit depth: 8, 10, 12 */
  int bd;
  DECLARE_ALIGNED(16, uint16_t, mc_buf_high[(CODING_UNIT_SIZE + 16) * 2 *
                                            (CODING_UNIT_SIZE + 16) * 2]);
#endif

  int lossless;

  int corrupted;

  DECLARE_ALIGNED(16, tran_low_t, dqcoeff[MAX_MB_PLANE][CODING_UNIT_SIZE *
                                                        CODING_UNIT_SIZE]);
#if CONFIG_PALETTE
  DECLARE_ALIGNED(16, uint8_t, color_index_map[2][CODING_UNIT_SIZE *
                                                  CODING_UNIT_SIZE]);
  DECLARE_ALIGNED(16, uint8_t, palette_map_buffer[CODING_UNIT_SIZE *
                                                  CODING_UNIT_SIZE]);
#endif  // CONFIG_PALETTE

  ENTROPY_CONTEXT *above_context[MAX_MB_PLANE];
  ENTROPY_CONTEXT left_context[MAX_MB_PLANE][2 * MI_BLOCK_SIZE];

  PARTITION_CONTEXT *above_seg_context;
  PARTITION_CONTEXT left_seg_context[MI_BLOCK_SIZE];
#if CONFIG_GLOBAL_MOTION
  Global_Motion_Params (*global_motion)[MAX_GLOBAL_MOTION_MODELS];
#endif  // CONFIG_GLOBAL_MOTION
} MACROBLOCKD;

static INLINE BLOCK_SIZE get_subsize(BLOCK_SIZE bsize,
                                     PARTITION_TYPE partition) {
  return subsize_lookup[partition][bsize];
}

#if CONFIG_EXT_PARTITION
static INLINE PARTITION_TYPE get_partition(const MODE_INFO *const mi,
                                           int mi_stride, int mi_rows,
                                           int mi_cols, int mi_row,
                                           int mi_col, BLOCK_SIZE bsize) {
  const int bsl = b_width_log2_lookup[bsize];
  const int bs = (1 << bsl) / 4;
  MODE_INFO *m = mi[mi_row * mi_stride + mi_col].src_mi;
  PARTITION_TYPE partition = partition_lookup[bsl][m->mbmi.sb_type];
  if (partition != PARTITION_NONE && bsize > BLOCK_8X8 &&
      mi_row + bs < mi_rows && mi_col + bs < mi_cols) {
    BLOCK_SIZE h = get_subsize(bsize, PARTITION_HORZ_A);
    BLOCK_SIZE v = get_subsize(bsize, PARTITION_VERT_A);
    MODE_INFO *m_right = mi[mi_row * mi_stride + mi_col + bs].src_mi;
    MODE_INFO *m_below = mi[(mi_row + bs) * mi_stride + mi_col].src_mi;
    if (m->mbmi.sb_type == h) {
      return m_below->mbmi.sb_type == h ? PARTITION_HORZ : PARTITION_HORZ_B;
    } else if (m_below->mbmi.sb_type == h) {
      return m->mbmi.sb_type == h ? PARTITION_HORZ : PARTITION_HORZ_A;
    } else if (m->mbmi.sb_type == v) {
      return m_right->mbmi.sb_type == v ? PARTITION_VERT : PARTITION_VERT_B;
    } else if (m_right->mbmi.sb_type == v) {
      return m->mbmi.sb_type == v ? PARTITION_VERT : PARTITION_VERT_A;
    } else {
      return PARTITION_SPLIT;
    }
  }
  return partition;
}
#endif

extern const TX_TYPE intra_mode_to_tx_type_lookup[INTRA_MODES];

#if CONFIG_SUPERTX

#define PARTITION_SUPERTX_CONTEXTS 2

#if CONFIG_TX64X64
#define MAX_SUPERTX_BLOCK_SIZE BLOCK_64X64
#else
#define MAX_SUPERTX_BLOCK_SIZE BLOCK_32X32
#endif  // CONFIG_TX64X64

static INLINE TX_SIZE bsize_to_tx_size(BLOCK_SIZE bsize) {
  const TX_SIZE bsize_to_tx_size_lookup[BLOCK_SIZES] = {
    TX_4X4, TX_4X4, TX_4X4,
    TX_8X8, TX_8X8, TX_8X8,
    TX_16X16, TX_16X16, TX_16X16,
    TX_32X32, TX_32X32, TX_32X32,
#if CONFIG_TX64X64
    TX_64X64
#if CONFIG_EXT_CODING_UNIT_SIZE
    , TX_64X64, TX_64X64, TX_64X64
#endif  // CONFIG_EXT_CODING_UNIT_SIZE
#else
    TX_32X32
#if CONFIG_EXT_CODING_UNIT_SIZE
    , TX_32X32, TX_32X32, TX_32X32
#endif  // CONFIG_EXT_CODING_UNIT_SIZE
#endif  // CONFIG_TX64X64
  };
  return bsize_to_tx_size_lookup[bsize];
}

static INLINE int supertx_enabled(const MB_MODE_INFO *mbmi) {
  return (int)mbmi->tx_size >
         MIN(b_width_log2_lookup[mbmi->sb_type],
             b_height_log2_lookup[mbmi->sb_type]);
}
#endif  // CONFIG_SUPERTX

#if CONFIG_EXT_TX
#if CONFIG_WAVELETS
#define GET_EXT_TX_TYPES(tx_size) \
    ((tx_size) >= TX_32X32 ? EXT_TX_TYPES_LARGE : EXT_TX_TYPES)
#define GET_EXT_TX_TREE(tx_size) \
    ((tx_size) >= TX_32X32 ? vp9_ext_tx_large_tree : vp9_ext_tx_tree)
#define GET_EXT_TX_ENCODINGS(tx_size) \
    ((tx_size) >= TX_32X32 ? ext_tx_large_encodings : ext_tx_encodings)
#else
#define GET_EXT_TX_TYPES(tx_size) \
    ((tx_size) >= TX_32X32 ? 1 : EXT_TX_TYPES)
#define GET_EXT_TX_TREE(tx_size) \
    ((tx_size) >= TX_32X32 ? NULL : vp9_ext_tx_tree)
#define GET_EXT_TX_ENCODINGS(tx_size) \
    ((tx_size) >= TX_32X32 ? NULL : ext_tx_encodings)
#endif  // CONFIG_WAVELETS

static TX_TYPE ext_tx_to_txtype[EXT_TX_TYPES] = {
  DCT_DCT,
  ADST_DCT,
  DCT_ADST,
  ADST_ADST,
  FLIPADST_DCT,
  DCT_FLIPADST,
  FLIPADST_FLIPADST,
  ADST_FLIPADST,
  FLIPADST_ADST,
  DST_DST,
  DST_DCT,
  DCT_DST,
  DST_ADST,
  ADST_DST,
  DST_FLIPADST,
  FLIPADST_DST,
};

static INLINE int is_dst_used(TX_TYPE tx_type) {
  return (tx_type == DST_DST ||
          tx_type == DST_DCT || tx_type == DCT_DST ||
          tx_type == DST_ADST || tx_type == ADST_DST ||
          tx_type == DST_FLIPADST || tx_type == FLIPADST_DST);
}

#if CONFIG_WAVELETS
static TX_TYPE ext_tx_to_txtype_large[EXT_TX_TYPES_LARGE] = {
  DCT_DCT,
  WAVELET1_DCT_DCT
};
#endif  // CONFIG_WAVELETS
#endif  // CONFIG_EXT_TX

static INLINE TX_TYPE get_tx_type_large(PLANE_TYPE plane_type,
                                        const MACROBLOCKD *xd) {
#if CONFIG_EXT_TX && CONFIG_WAVELETS
  const MB_MODE_INFO *const mbmi = &xd->mi[0].src_mi->mbmi;
  if (plane_type != PLANE_TYPE_Y || xd->lossless)
      return DCT_DCT;

  if (is_inter_block(mbmi)) {
    return ext_tx_to_txtype_large[mbmi->ext_txfrm];
  }
#endif  // CONFIG_EXT_TX  && CONFIG_WAVELETS
  (void) plane_type;
  (void) xd;
  return DCT_DCT;
}

static INLINE TX_TYPE get_tx_type(PLANE_TYPE plane_type,
                                  const MACROBLOCKD *xd) {
  const MB_MODE_INFO *const mbmi = &xd->mi[0].src_mi->mbmi;
  (void) plane_type;

#if CONFIG_EXT_TX
  if (xd->lossless)
      return DCT_DCT;

  if (is_inter_block(mbmi)) {
    return ext_tx_to_txtype[mbmi->ext_txfrm];
  }
#if CONFIG_INTRABC
  if (is_intrabc_mode(mbmi->mode))
    return DCT_DCT;
#endif  // CONFIG_INTRABC
  return intra_mode_to_tx_type_lookup[plane_type == PLANE_TYPE_Y ?
      mbmi->mode : mbmi->uv_mode];
#else   // CONFIG_EXT_TX
  if (plane_type != PLANE_TYPE_Y || xd->lossless || is_inter_block(mbmi))
    return DCT_DCT;
#if CONFIG_INTRABC
  if (is_intrabc_mode(mbmi->mode))
    return DCT_DCT;
#endif  // CONFIG_INTRABC
  return intra_mode_to_tx_type_lookup[mbmi->mode];
#endif  // CONFIG_EXT_TX
}

static INLINE TX_TYPE get_tx_type_4x4(PLANE_TYPE plane_type,
                                      const MACROBLOCKD *xd, int ib) {
  const MODE_INFO *const mi = xd->mi[0].src_mi;
  PREDICTION_MODE mode;
  (void) plane_type;

#if CONFIG_EXT_TX
  if (xd->lossless)
      return DCT_DCT;

  if (is_inter_block(&mi->mbmi)) {
    return ext_tx_to_txtype[mi->mbmi.ext_txfrm];
  }
  mode = get_y_mode(mi, ib);
#if CONFIG_INTRABC
  if (is_intrabc_mode(mode))
    return DCT_DCT;
#endif  // CONFIG_INTRABC
  return intra_mode_to_tx_type_lookup[plane_type == PLANE_TYPE_Y ?
      mode : mi->mbmi.uv_mode];
#else   // CONFIG_EXT_TX
  if (plane_type != PLANE_TYPE_Y || xd->lossless || is_inter_block(&mi->mbmi))
    return DCT_DCT;
  mode = get_y_mode(mi, ib);
#if CONFIG_INTRABC
  if (is_intrabc_mode(mode))
    return DCT_DCT;
#endif  // CONFIG_INTRABC

  return intra_mode_to_tx_type_lookup[mode];
#endif  // CONFIG_EXT_TX
}

void vp9_setup_block_planes(MACROBLOCKD *xd, int ss_x, int ss_y);

static INLINE TX_SIZE get_uv_tx_size_impl(TX_SIZE y_tx_size, BLOCK_SIZE bsize,
                                          int xss, int yss) {
  if (bsize < BLOCK_8X8) {
    return TX_4X4;
  } else {
    const BLOCK_SIZE plane_bsize = ss_size_lookup[bsize][xss][yss];
    return MIN(y_tx_size, max_txsize_lookup[plane_bsize]);
  }
}

static INLINE TX_SIZE get_uv_tx_size(const MB_MODE_INFO *mbmi,
                                     const struct macroblockd_plane *pd) {
#if CONFIG_SUPERTX
  if (!supertx_enabled(mbmi)) {
    return get_uv_tx_size_impl(mbmi->tx_size, mbmi->sb_type, pd->subsampling_x,
                               pd->subsampling_y);
  } else {
    return uvsupertx_size_lookup[mbmi->tx_size][pd->subsampling_x]
                                               [pd->subsampling_y];
  }
#else
  return get_uv_tx_size_impl(mbmi->tx_size, mbmi->sb_type, pd->subsampling_x,
                             pd->subsampling_y);
#endif  // CONFIG_SUPERTX
}

static INLINE BLOCK_SIZE get_plane_block_size(BLOCK_SIZE bsize,
    const struct macroblockd_plane *pd) {
  return ss_size_lookup[bsize][pd->subsampling_x][pd->subsampling_y];
}

typedef void (*foreach_transformed_block_visitor)(int plane, int block,
                                                  BLOCK_SIZE plane_bsize,
                                                  TX_SIZE tx_size,
                                                  void *arg);

void vp9_foreach_transformed_block_in_plane(
    const MACROBLOCKD *const xd, BLOCK_SIZE bsize, int plane,
    foreach_transformed_block_visitor visit, void *arg);


void vp9_foreach_transformed_block(
    const MACROBLOCKD* const xd, BLOCK_SIZE bsize,
    foreach_transformed_block_visitor visit, void *arg);

static INLINE void txfrm_block_to_raster_xy(BLOCK_SIZE plane_bsize,
                                            TX_SIZE tx_size, int block,
                                            int *x, int *y) {
  const int bwl = b_width_log2_lookup[plane_bsize];
  const int tx_cols_log2 = bwl - tx_size;
  const int tx_cols = 1 << tx_cols_log2;
  const int raster_mb = block >> (tx_size << 1);
  *x = (raster_mb & (tx_cols - 1)) << tx_size;
  *y = (raster_mb >> tx_cols_log2) << tx_size;
}

void vp9_set_contexts(const MACROBLOCKD *xd, struct macroblockd_plane *pd,
                      BLOCK_SIZE plane_bsize, TX_SIZE tx_size, int has_eob,
                      int aoff, int loff);

#if CONFIG_INTERINTRA
static INLINE int is_interintra_allowed(BLOCK_SIZE sb_type) {
  return ((sb_type >= BLOCK_8X8) && (sb_type < BLOCK_64X64));
}
#endif  // CONFIG_INTERINTRA

#if CONFIG_WEDGE_PARTITION
#define WEDGE_BITS_SML   3
#define WEDGE_BITS_MED   4
#define WEDGE_BITS_BIG   5
#define WEDGE_NONE      -1

#define WEDGE_WEIGHT_BITS 6

static INLINE int get_wedge_bits(BLOCK_SIZE sb_type) {
  if (sb_type < BLOCK_8X8)
    return 0;
  if (sb_type <= BLOCK_8X8)
    return WEDGE_BITS_SML;
  else if (sb_type <= BLOCK_32X32)
    return WEDGE_BITS_MED;
  else
    return WEDGE_BITS_BIG;
}
#endif  // CONFIG_WEDGE_PARTITION

#if CONFIG_NEW_QUANT && CONFIG_TX_SKIP
static INLINE int is_rect_quant_used(const MB_MODE_INFO *mbmi,
                                     int plane) {
  return
      mbmi->tx_skip[plane != 0] &&
      ((plane == 0 && (mbmi->mode == V_PRED ||
                       mbmi->mode == H_PRED ||
                       mbmi->mode == TM_PRED)) ||
       (plane != 0 && (mbmi->uv_mode == V_PRED ||
                       mbmi->uv_mode == H_PRED ||
                       mbmi->uv_mode == TM_PRED)));
}
#endif  // CONFIG_NEW_QUANT && CONFIG_TX_SKIP

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP9_COMMON_VP9_BLOCKD_H_
