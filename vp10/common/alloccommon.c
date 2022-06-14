/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./vpx_config.h"
#include "vpx_mem/vpx_mem.h"

#include "vp10/common/alloccommon.h"
#include "vp10/common/blockd.h"
#include "vp10/common/entropymode.h"
#include "vp10/common/entropymv.h"
#include "vp10/common/onyxc_int.h"

void vp10_set_mb_mi(VP10_COMMON *cm, int width, int height) {
  const int aligned_width = ALIGN_POWER_OF_TWO(width, MI_SIZE_LOG2);
  const int aligned_height = ALIGN_POWER_OF_TWO(height, MI_SIZE_LOG2);

  cm->mi_cols = aligned_width >> MI_SIZE_LOG2;
  cm->mi_rows = aligned_height >> MI_SIZE_LOG2;
  cm->mi_stride = calc_mi_size(cm->mi_cols);

  cm->mb_cols = (cm->mi_cols + 1) >> 1;
  cm->mb_rows = (cm->mi_rows + 1) >> 1;
  cm->MBs = cm->mb_rows * cm->mb_cols;
}

static int alloc_seg_map(VP10_COMMON *cm, int seg_map_size) {
  int i;

  for (i = 0; i < NUM_PING_PONG_BUFFERS; ++i) {
    cm->seg_map_array[i] = (uint8_t *)vpx_calloc(seg_map_size, 1);
    if (cm->seg_map_array[i] == NULL)
      return 1;
  }
  cm->seg_map_alloc_size = seg_map_size;

  // Init the index.
  cm->seg_map_idx = 0;
  cm->prev_seg_map_idx = 1;

  cm->current_frame_seg_map = cm->seg_map_array[cm->seg_map_idx];
  if (!cm->frame_parallel_decode)
    cm->last_frame_seg_map = cm->seg_map_array[cm->prev_seg_map_idx];

  return 0;
}

static void free_seg_map(VP10_COMMON *cm) {
  int i;

  for (i = 0; i < NUM_PING_PONG_BUFFERS; ++i) {
    vpx_free(cm->seg_map_array[i]);
    cm->seg_map_array[i] = NULL;
  }

  cm->current_frame_seg_map = NULL;

  if (!cm->frame_parallel_decode) {
    cm->last_frame_seg_map = NULL;
  }
}

void vp10_free_ref_frame_buffers(BufferPool *pool) {
  int i;

  for (i = 0; i < FRAME_BUFFERS; ++i) {
    if (pool->frame_bufs[i].ref_count > 0 &&
        pool->frame_bufs[i].raw_frame_buffer.data != NULL) {
      pool->release_fb_cb(pool->cb_priv, &pool->frame_bufs[i].raw_frame_buffer);
      pool->frame_bufs[i].ref_count = 0;
    }
    vpx_free(pool->frame_bufs[i].mvs);
    pool->frame_bufs[i].mvs = NULL;
    vpx_free_frame_buffer(&pool->frame_bufs[i].buf);
  }
}

#if CONFIG_LOOP_RESTORATION
void vp10_free_restoration_buffers(VP10_COMMON *cm) {
  vpx_free_frame_buffer(&cm->tmp_loop_buf);
}
#endif  // CONFIG_LOOP_RESTORATION

void vp10_free_context_buffers(VP10_COMMON *cm) {
  int i;
  cm->free_mi(cm);
  free_seg_map(cm);
  for (i = 0 ; i < MAX_MB_PLANE ; i++) {
    vpx_free(cm->above_context[i]);
    cm->above_context[i] = NULL;
  }
  vpx_free(cm->above_seg_context);
  cm->above_seg_context = NULL;
#if CONFIG_VAR_TX
  vpx_free(cm->above_txfm_context);
  cm->above_txfm_context = NULL;
#endif
}

int vp10_alloc_context_buffers(VP10_COMMON *cm, int width, int height) {
  int new_mi_size;

  vp10_set_mb_mi(cm, width, height);
  new_mi_size = cm->mi_stride * calc_mi_size(cm->mi_rows);
  if (cm->mi_alloc_size < new_mi_size) {
    cm->free_mi(cm);
    if (cm->alloc_mi(cm, new_mi_size))
      goto fail;
  }

  if (cm->seg_map_alloc_size < cm->mi_rows * cm->mi_cols) {
    // Create the segmentation map structure and set to 0.
    free_seg_map(cm);
    if (alloc_seg_map(cm, cm->mi_rows * cm->mi_cols))
      goto fail;
  }

  if (cm->above_context_alloc_cols < cm->mi_cols) {
    // TODO(geza.lore): These are bigger than they need to be.
    // cm->tile_width would be enough but it complicates indexing a
    // little elsewhere.
    const int aligned_mi_cols =
        ALIGN_POWER_OF_TWO(cm->mi_cols, MAX_MIB_SIZE_LOG2);
    int i;

    for (i = 0 ; i < MAX_MB_PLANE ; i++) {
      vpx_free(cm->above_context[i]);
      cm->above_context[i] = (ENTROPY_CONTEXT *)vpx_calloc(
          2 * aligned_mi_cols, sizeof(*cm->above_context[0]));
      if (!cm->above_context[i]) goto fail;
    }

    vpx_free(cm->above_seg_context);
    cm->above_seg_context = (PARTITION_CONTEXT *)vpx_calloc(
        aligned_mi_cols, sizeof(*cm->above_seg_context));
    if (!cm->above_seg_context) goto fail;

#if CONFIG_VAR_TX
    vpx_free(cm->above_txfm_context);
    cm->above_txfm_context = (TXFM_CONTEXT *)vpx_calloc(
        aligned_mi_cols, sizeof(*cm->above_txfm_context));
    if (!cm->above_txfm_context) goto fail;
#endif

    cm->above_context_alloc_cols = aligned_mi_cols;
  }

  return 0;

 fail:
  vp10_free_context_buffers(cm);
  return 1;
}

void vp10_remove_common(VP10_COMMON *cm) {
  vp10_free_context_buffers(cm);

  vpx_free(cm->fc);
  cm->fc = NULL;
  vpx_free(cm->frame_contexts);
  cm->frame_contexts = NULL;
}

void vp10_init_context_buffers(VP10_COMMON *cm) {
  cm->setup_mi(cm);
  if (cm->last_frame_seg_map && !cm->frame_parallel_decode)
    memset(cm->last_frame_seg_map, 0, cm->mi_rows * cm->mi_cols);
}

void vp10_swap_current_and_last_seg_map(VP10_COMMON *cm) {
  // Swap indices.
  const int tmp = cm->seg_map_idx;
  cm->seg_map_idx = cm->prev_seg_map_idx;
  cm->prev_seg_map_idx = tmp;

  cm->current_frame_seg_map = cm->seg_map_array[cm->seg_map_idx];
  cm->last_frame_seg_map = cm->seg_map_array[cm->prev_seg_map_idx];
}
