/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP9_COMMON_VP9_ONYXC_INT_H_
#define VP9_COMMON_VP9_ONYXC_INT_H_

#include "./vpx_config.h"
#include "vpx/internal/vpx_codec_internal.h"
#include "./vp9_rtcd.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/common/vp9_entropy.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_tile_common.h"

#if CONFIG_VP9_POSTPROC
#include "vp9/common/vp9_postproc.h"
#endif

#define REFS_PER_FRAME 3

#define REF_FRAMES_LOG2 3
#define REF_FRAMES (1 << REF_FRAMES_LOG2)

// 1 scratch frame for the new frame, 3 for scaled references on the encoder
// TODO(jkoleszar): These 3 extra references could probably come from the
// normal reference pool.
#define FRAME_BUFFERS (REF_FRAMES + 4)

#define FRAME_CONTEXTS_LOG2 2
#define FRAME_CONTEXTS (1 << FRAME_CONTEXTS_LOG2)

extern const struct {
  PARTITION_CONTEXT above;
  PARTITION_CONTEXT left;
} partition_context_lookup[BLOCK_SIZES];

typedef struct frame_contexts {
  vp9_prob y_mode_prob[BLOCK_SIZE_GROUPS][INTRA_MODES - 1];
  vp9_prob uv_mode_prob[INTRA_MODES][INTRA_MODES - 1];
  vp9_prob partition_prob[PARTITION_CONTEXTS][PARTITION_TYPES - 1];
  vp9_coeff_probs_model coef_probs[TX_SIZES][PLANE_TYPES];
  vp9_prob switchable_interp_prob[SWITCHABLE_FILTER_CONTEXTS]
                                 [SWITCHABLE_FILTERS - 1];
  vp9_prob inter_mode_probs[INTER_MODE_CONTEXTS][INTER_MODES - 1];
  vp9_prob intra_inter_prob[INTRA_INTER_CONTEXTS];
  vp9_prob comp_inter_prob[COMP_INTER_CONTEXTS];
  vp9_prob single_ref_prob[REF_CONTEXTS][2];
  vp9_prob comp_ref_prob[REF_CONTEXTS];
  struct tx_probs tx_probs;
  vp9_prob mbskip_probs[MBSKIP_CONTEXTS];
  nmv_context nmvc;
#if CONFIG_FILTERINTRA
  vp9_prob filterintra_prob[TX_SIZES][INTRA_MODES];
#endif
#if CONFIG_MASKED_INTERINTER
  vp9_prob masked_interinter_prob[BLOCK_SIZES];
#endif
#if CONFIG_INTERINTRA
  vp9_prob interintra_prob[BLOCK_SIZES];
#if CONFIG_MASKED_INTERINTRA
  vp9_prob masked_interintra_prob[BLOCK_SIZES];
#endif
#endif
#if CONFIG_EXT_TX
  vp9_prob ext_tx_prob;
#endif
} FRAME_CONTEXT;

typedef struct {
  unsigned int y_mode[BLOCK_SIZE_GROUPS][INTRA_MODES];
  unsigned int uv_mode[INTRA_MODES][INTRA_MODES];
  unsigned int partition[PARTITION_CONTEXTS][PARTITION_TYPES];
  vp9_coeff_count_model coef[TX_SIZES][PLANE_TYPES];
  unsigned int eob_branch[TX_SIZES][PLANE_TYPES][REF_TYPES]
                         [COEF_BANDS][COEFF_CONTEXTS];
  unsigned int switchable_interp[SWITCHABLE_FILTER_CONTEXTS]
                                [SWITCHABLE_FILTERS];
  unsigned int inter_mode[INTER_MODE_CONTEXTS][INTER_MODES];
  unsigned int intra_inter[INTRA_INTER_CONTEXTS][2];
  unsigned int comp_inter[COMP_INTER_CONTEXTS][2];
  unsigned int single_ref[REF_CONTEXTS][2][2];
  unsigned int comp_ref[REF_CONTEXTS][2];
  struct tx_counts tx;
  unsigned int mbskip[MBSKIP_CONTEXTS][2];
  nmv_context_counts mv;
#if CONFIG_FILTERINTRA
  unsigned int filterintra[TX_SIZES][INTRA_MODES][2];
#endif
#if CONFIG_MASKED_INTERINTER
  unsigned int masked_interinter[BLOCK_SIZES][2];
#endif
#if CONFIG_INTERINTRA
  unsigned int interintra[BLOCK_SIZES][2];
#if CONFIG_MASKED_INTERINTRA
  unsigned int masked_interintra[BLOCK_SIZES][2];
#endif
#endif
#if CONFIG_EXT_TX
  unsigned int ext_tx[2];
#endif
} FRAME_COUNTS;


typedef enum {
  SINGLE_REFERENCE      = 0,
  COMPOUND_REFERENCE    = 1,
  REFERENCE_MODE_SELECT = 2,
  REFERENCE_MODES       = 3,
} REFERENCE_MODE;

typedef struct VP9Common {
  struct vpx_internal_error_info  error;

  DECLARE_ALIGNED(16, int16_t, y_dequant[QINDEX_RANGE][8]);
  DECLARE_ALIGNED(16, int16_t, uv_dequant[QINDEX_RANGE][8]);
#if CONFIG_ALPHA
  DECLARE_ALIGNED(16, int16_t, a_dequant[QINDEX_RANGE][8]);
#endif

  COLOR_SPACE color_space;

  int width;
  int height;
  int display_width;
  int display_height;
  int last_width;
  int last_height;

  // TODO(jkoleszar): this implies chroma ss right now, but could vary per
  // plane. Revisit as part of the future change to YV12_BUFFER_CONFIG to
  // support additional planes.
  int subsampling_x;
  int subsampling_y;

  YV12_BUFFER_CONFIG *frame_to_show;

  YV12_BUFFER_CONFIG *yv12_fb;
  int *fb_idx_ref_cnt; /* reference counts */
  int ref_frame_map[REF_FRAMES]; /* maps fb_idx to reference slot */

  // TODO(jkoleszar): could expand active_ref_idx to 4, with 0 as intra, and
  // roll new_fb_idx into it.

  // Each frame can reference REFS_PER_FRAME buffers
  int active_ref_idx[REFS_PER_FRAME];
  struct scale_factors active_ref_scale[REFS_PER_FRAME];
  struct scale_factors_common active_ref_scale_comm[REFS_PER_FRAME];
  int new_fb_idx;

  YV12_BUFFER_CONFIG post_proc_buffer;

  FRAME_TYPE last_frame_type;  /* last frame's frame type for motion search.*/
  FRAME_TYPE frame_type;

  int show_frame;
  int last_show_frame;

  // Flag signaling that the frame is encoded using only INTRA modes.
  int intra_only;

  int allow_high_precision_mv;

  // Flag signaling that the frame context should be reset to default values.
  // 0 or 1 implies don't reset, 2 reset just the context specified in the
  // frame header, 3 reset all contexts.
  int reset_frame_context;

  int frame_flags;
  // MBs, mb_rows/cols is in 16-pixel units; mi_rows/cols is in
  // MODE_INFO (8-pixel) units.
  int MBs;
  int mb_rows, mi_rows;
  int mb_cols, mi_cols;
  int mode_info_stride;

  /* profile settings */
  TX_MODE tx_mode;

  int base_qindex;
  int y_dc_delta_q;
  int uv_dc_delta_q;
  int uv_ac_delta_q;
#if CONFIG_ALPHA
  int a_dc_delta_q;
  int a_ac_delta_q;
#endif

  /* We allocate a MODE_INFO struct for each macroblock, together with
     an extra row on top and column on the left to simplify prediction. */

  MODE_INFO *mip; /* Base of allocated array */
  MODE_INFO *mi;  /* Corresponds to upper left visible macroblock */
  MODE_INFO *prev_mip; /* MODE_INFO array 'mip' from last decoded frame */
  MODE_INFO *prev_mi;  /* 'mi' from last frame (points into prev_mip) */

  MODE_INFO **mi_grid_base;
  MODE_INFO **mi_grid_visible;
  MODE_INFO **prev_mi_grid_base;
  MODE_INFO **prev_mi_grid_visible;

  // Persistent mb segment id map used in prediction.
  unsigned char *last_frame_seg_map;

  INTERPOLATION_TYPE mcomp_filter_type;

  loop_filter_info_n lf_info;

  int refresh_frame_context;    /* Two state 0 = NO, 1 = YES */

  int ref_frame_sign_bias[MAX_REF_FRAMES];    /* Two state 0, 1 */

  struct loopfilter lf;
  struct segmentation seg;

  // Context probabilities for reference frame prediction
  int allow_comp_inter_inter;
  MV_REFERENCE_FRAME comp_fixed_ref;
  MV_REFERENCE_FRAME comp_var_ref[2];
  REFERENCE_MODE reference_mode;

  FRAME_CONTEXT fc;  /* this frame entropy */
  FRAME_CONTEXT frame_contexts[FRAME_CONTEXTS];
  unsigned int  frame_context_idx; /* Context to use/update */
  FRAME_COUNTS counts;

  unsigned int current_video_frame;
  int version;

#if CONFIG_VP9_POSTPROC
  struct postproc_state  postproc_state;
#endif

  int error_resilient_mode;
  int frame_parallel_decoding_mode;

  int log2_tile_cols, log2_tile_rows;

  vpx_codec_frame_buffer_t *fb_list;  // External frame buffers
  int fb_count;  // Total number of frame buffers
  vpx_realloc_frame_buffer_cb_fn_t realloc_fb_cb;
  void *user_priv;  // Private data associated with the external frame buffers.

  int fb_lru;  // Flag telling if lru is on/off
  uint32_t *fb_idx_ref_lru;  // Frame buffer lru cache
  uint32_t fb_idx_ref_lru_count;
#if CONFIG_MASKED_INTERINTER
  int use_masked_interinter;
#endif
#if CONFIG_INTERINTRA
  int use_interintra;
#if CONFIG_MASKED_INTERINTRA
  int use_masked_interintra;
#endif
#endif
} VP9_COMMON;

// ref == 0 => LAST_FRAME
// ref == 1 => GOLDEN_FRAME
// ref == 2 => ALTREF_FRAME
static YV12_BUFFER_CONFIG *get_frame_ref_buffer(VP9_COMMON *cm, int ref) {
  return &cm->yv12_fb[cm->active_ref_idx[ref]];
}

static YV12_BUFFER_CONFIG *get_frame_new_buffer(VP9_COMMON *cm) {
  return &cm->yv12_fb[cm->new_fb_idx];
}

static int get_free_fb(VP9_COMMON *cm) {
  int i;
  uint32_t lru_count = cm->fb_idx_ref_lru_count + 1;
  int free_buffer_idx = cm->fb_count;
  for (i = 0; i < cm->fb_count; i++) {
    if (!cm->fb_lru) {
      if (cm->fb_idx_ref_cnt[i] == 0) {
        free_buffer_idx = i;
        break;
      }
    } else {
      if (cm->fb_idx_ref_cnt[i] == 0 && cm->fb_idx_ref_lru[i] < lru_count) {
        free_buffer_idx = i;
        lru_count = cm->fb_idx_ref_lru[i];
      }
    }
  }

  assert(free_buffer_idx < cm->fb_count);
  cm->fb_idx_ref_cnt[free_buffer_idx] = 1;
  if (cm->fb_lru)
    cm->fb_idx_ref_lru[free_buffer_idx] = ++cm->fb_idx_ref_lru_count;
  return free_buffer_idx;
}

static void ref_cnt_fb(int *buf, int *idx, int new_idx) {
  const int ref_index = *idx;

  if (ref_index >= 0 && buf[ref_index] > 0)
    buf[ref_index]--;

  *idx = new_idx;

  buf[new_idx]++;
}

static int mi_cols_aligned_to_sb(int n_mis) {
  return ALIGN_POWER_OF_TWO(n_mis, MI_BLOCK_SIZE_LOG2);
}

static INLINE const vp9_prob* get_partition_probs(VP9_COMMON *cm, int ctx) {
  return cm->frame_type == KEY_FRAME ? vp9_kf_partition_probs[ctx]
                                     : cm->fc.partition_prob[ctx];
}

static INLINE void set_skip_context(
    MACROBLOCKD *xd,
    ENTROPY_CONTEXT *above_context[MAX_MB_PLANE],
    ENTROPY_CONTEXT left_context[MAX_MB_PLANE][16],
    int mi_row, int mi_col) {
  const int above_idx = mi_col * 2;
  const int left_idx = (mi_row * 2) & 15;
  int i;
  for (i = 0; i < MAX_MB_PLANE; i++) {
    struct macroblockd_plane *const pd = &xd->plane[i];
    pd->above_context = above_context[i] + (above_idx >> pd->subsampling_x);
    pd->left_context = left_context[i] + (left_idx >> pd->subsampling_y);
  }
}

static void set_mi_row_col(MACROBLOCKD *xd, const TileInfo *const tile,
                           int mi_row, int bh,
                           int mi_col, int bw,
                           int mi_rows, int mi_cols) {
  xd->mb_to_top_edge    = -((mi_row * MI_SIZE) * 8);
  xd->mb_to_bottom_edge = ((mi_rows - bh - mi_row) * MI_SIZE) * 8;
  xd->mb_to_left_edge   = -((mi_col * MI_SIZE) * 8);
  xd->mb_to_right_edge  = ((mi_cols - bw - mi_col) * MI_SIZE) * 8;

  // Are edges available for intra prediction?
  xd->up_available    = (mi_row != 0);
  xd->left_available  = (mi_col > tile->mi_col_start);
}

static void set_prev_mi(VP9_COMMON *cm) {
  const int use_prev_in_find_mv_refs = cm->width == cm->last_width &&
                                       cm->height == cm->last_height &&
                                       !cm->error_resilient_mode &&
                                       !cm->intra_only &&
                                       cm->last_show_frame;
  // Special case: set prev_mi to NULL when the previous mode info
  // context cannot be used.
  cm->prev_mi = use_prev_in_find_mv_refs ?
                  cm->prev_mip + cm->mode_info_stride + 1 : NULL;
}

static INLINE int frame_is_intra_only(const VP9_COMMON *const cm) {
  return cm->frame_type == KEY_FRAME || cm->intra_only;
}

static INLINE void update_partition_context(
    PARTITION_CONTEXT *above_seg_context,
    PARTITION_CONTEXT left_seg_context[8],
    int mi_row, int mi_col, BLOCK_SIZE subsize, BLOCK_SIZE bsize) {
  PARTITION_CONTEXT *const above_ctx = above_seg_context + mi_col;
  PARTITION_CONTEXT *const left_ctx = left_seg_context + (mi_row & MI_MASK);

  // num_4x4_blocks_wide_lookup[bsize] / 2
  const int bs = num_8x8_blocks_wide_lookup[bsize];

  // update the partition context at the end notes. set partition bits
  // of block sizes larger than the current one to be one, and partition
  // bits of smaller block sizes to be zero.
  vpx_memset(above_ctx, partition_context_lookup[subsize].above, bs);
  vpx_memset(left_ctx, partition_context_lookup[subsize].left, bs);
}

static INLINE int partition_plane_context(
    const PARTITION_CONTEXT *above_seg_context,
    const PARTITION_CONTEXT left_seg_context[8],
    int mi_row, int mi_col, BLOCK_SIZE bsize) {
  const PARTITION_CONTEXT *above_ctx = above_seg_context + mi_col;
  const PARTITION_CONTEXT *left_ctx = left_seg_context + (mi_row & MI_MASK);

  const int bsl = mi_width_log2(bsize);
  const int bs = 1 << bsl;
  int above = 0, left = 0, i;

  assert(mi_width_log2(bsize) == mi_height_log2(bsize));
  assert(bsl >= 0);

  for (i = 0; i < bs; i++) {
    above |= above_ctx[i];
    left |= left_ctx[i];
  }
  above = (above & bs) > 0;
  left  = (left & bs) > 0;

  return (left * 2 + above) + bsl * PARTITION_PLOFFSET;
}

#endif  // VP9_COMMON_VP9_ONYXC_INT_H_
