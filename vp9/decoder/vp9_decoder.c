/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "./vpx_scale_rtcd.h"

#include "vpx_mem/vpx_mem.h"
#include "vpx_ports/vpx_timer.h"
#include "vpx_scale/vpx_scale.h"

#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_onyxc_int.h"
#if CONFIG_VP9_POSTPROC
#include "vp9/common/vp9_postproc.h"
#endif
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_systemdependent.h"

#include "vp9/decoder/vp9_decodeframe.h"
#include "vp9/decoder/vp9_decoder.h"
#include "vp9/decoder/vp9_detokenize.h"
#include "vp9/decoder/vp9_dthread.h"

static void initialize_dec() {
  static int init_done = 0;

  if (!init_done) {
    vp9_init_neighbors();
    init_done = 1;
  }
}

VP9Decoder *vp9_decoder_create(BufferPool *const pool) {
  VP9Decoder *const pbi = vpx_memalign(32, sizeof(*pbi));
  VP9_COMMON *const cm = pbi ? &pbi->common : NULL;

  if (!cm)
    return NULL;

  vp9_zero(*pbi);

  if (setjmp(cm->error.jmp)) {
    cm->error.setjmp = 0;
    vp9_decoder_remove(pbi);
    return NULL;
  }

  cm->error.setjmp = 1;
  initialize_dec();

  vp9_rtcd();

  // Initialize the references to not point to any frame buffers.
  vpx_memset(&cm->ref_frame_map, -1, sizeof(cm->ref_frame_map));
  vpx_memset(&cm->next_ref_frame_map, -1, sizeof(cm->next_ref_frame_map));

  cm->current_video_frame = 0;
  pbi->ready_for_new_data = 1;
  pbi->common.buffer_pool = pool;

  // vp9_init_dequantizer() is first called here. Add check in
  // frame_init_dequantizer() to avoid unnecessary calling of
  // vp9_init_dequantizer() for every frame.
  vp9_init_dequantizer(cm);

  vp9_loop_filter_init(cm);

  cm->error.setjmp = 0;

  vp9_get_worker_interface()->init(&pbi->lf_worker);

  return pbi;
}

void vp9_decoder_remove(VP9Decoder *pbi) {
  VP9_COMMON *const cm = &pbi->common;
  int i;

  vp9_remove_common(cm);
  vp9_get_worker_interface()->end(&pbi->lf_worker);
  vpx_free(pbi->lf_worker.data1);
  vpx_free(pbi->tile_data);
  for (i = 0; i < pbi->num_tile_workers; ++i) {
    VP9Worker *const worker = &pbi->tile_workers[i];
    vp9_get_worker_interface()->end(worker);
    vpx_free(worker->data1);
    vpx_free(worker->data2);
  }
  vpx_free(pbi->tile_workers);

  if (pbi->num_tile_workers) {
    const int sb_rows =
        mi_cols_aligned_to_sb(cm->mi_rows) >> MI_BLOCK_SIZE_LOG2;
    vp9_loop_filter_dealloc(&pbi->lf_row_sync, sb_rows);
  }

  vpx_free(pbi);
}

static int equal_dimensions(const YV12_BUFFER_CONFIG *a,
                            const YV12_BUFFER_CONFIG *b) {
    return a->y_height == b->y_height && a->y_width == b->y_width &&
           a->uv_height == b->uv_height && a->uv_width == b->uv_width;
}

vpx_codec_err_t vp9_copy_reference_dec(VP9Decoder *pbi,
                                       VP9_REFFRAME ref_frame_flag,
                                       YV12_BUFFER_CONFIG *sd) {
  VP9_COMMON *cm = &pbi->common;

  /* TODO(jkoleszar): The decoder doesn't have any real knowledge of what the
   * encoder is using the frame buffers for. This is just a stub to keep the
   * vpxenc --test-decode functionality working, and will be replaced in a
   * later commit that adds VP9-specific controls for this functionality.
   */
  if (ref_frame_flag == VP9_LAST_FLAG) {
    const YV12_BUFFER_CONFIG *const cfg =
        &cm->buffer_pool->frame_bufs[cm->ref_frame_map[0]].buf;
    if (!equal_dimensions(cfg, sd))
      vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                         "Incorrect buffer dimensions");
    else
      vp8_yv12_copy_frame(cfg, sd);
  } else {
    vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                       "Invalid reference frame");
  }

  return cm->error.error_code;
}


vpx_codec_err_t vp9_set_reference_dec(VP9_COMMON *cm,
                                      VP9_REFFRAME ref_frame_flag,
                                      YV12_BUFFER_CONFIG *sd) {
  RefBuffer *ref_buf = NULL;
  RefCntBuffer *const frame_bufs = cm->buffer_pool->frame_bufs;

  // TODO(jkoleszar): The decoder doesn't have any real knowledge of what the
  // encoder is using the frame buffers for. This is just a stub to keep the
  // vpxenc --test-decode functionality working, and will be replaced in a
  // later commit that adds VP9-specific controls for this functionality.
  if (ref_frame_flag == VP9_LAST_FLAG) {
    ref_buf = &cm->frame_refs[0];
  } else if (ref_frame_flag == VP9_GOLD_FLAG) {
    ref_buf = &cm->frame_refs[1];
  } else if (ref_frame_flag == VP9_ALT_FLAG) {
    ref_buf = &cm->frame_refs[2];
  } else {
    vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                       "Invalid reference frame");
    return cm->error.error_code;
  }

  if (!equal_dimensions(ref_buf->buf, sd)) {
    vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                       "Incorrect buffer dimensions");
  } else {
    int *ref_fb_ptr = &ref_buf->idx;

    // Find an empty frame buffer.
    const int free_fb = get_free_fb(cm);
    // Decrease ref_count since it will be increased again in
    // ref_cnt_fb() below.
    --frame_bufs[free_fb].ref_count;

    // Manage the reference counters and copy image.
    ref_cnt_fb(frame_bufs, ref_fb_ptr, free_fb);
    ref_buf->buf = &frame_bufs[*ref_fb_ptr].buf;
    vp8_yv12_copy_frame(sd, ref_buf->buf);
  }

  return cm->error.error_code;
}


int vp9_get_reference_dec(VP9Decoder *pbi, int index, YV12_BUFFER_CONFIG **fb) {
  VP9_COMMON *cm = &pbi->common;
  RefCntBuffer *const frame_bufs = cm->buffer_pool->frame_bufs;

  if (index < 0 || index >= REF_FRAMES)
    return -1;

  *fb = &frame_bufs[cm->ref_frame_map[index]].buf;
  return 0;
}

/* If any buffer updating is signaled it should be done here. */
static void swap_frame_buffers(VP9Decoder *pbi) {
  int ref_index = 0, mask;
  VP9_COMMON * const cm = &pbi->common;
  BufferPool * const pool = cm->buffer_pool;
  RefCntBuffer *const frame_bufs = cm->buffer_pool->frame_bufs;

  for (mask = pbi->refresh_frame_flags; mask; mask >>= 1) {
    if (mask & 1) {
      const int old_idx = cm->ref_frame_map[ref_index];
#if CONFIG_MULTITHREAD
      pthread_mutex_lock(&cm->buffer_pool->pool_mutex);
#endif
      ref_cnt_fb(frame_bufs, &cm->ref_frame_map[ref_index],
                 cm->new_fb_idx);
      if (old_idx >= 0 && frame_bufs[old_idx].ref_count == 0)
        pool->release_fb_cb(pool->cb_priv,
                            &frame_bufs[old_idx].raw_frame_buffer);
    }
#if CONFIG_MULTITHREAD
      pthread_mutex_unlock(&cm->buffer_pool->pool_mutex);
#endif
    ++ref_index;
  }

  cm->frame_to_show = get_frame_new_buffer(cm);

  if (!pbi->frame_parallel_decode || !cm->show_frame) {
    --frame_bufs[cm->new_fb_idx].ref_count;
  }

  // Invalidate these references until the next frame starts.
  for (ref_index = 0; ref_index < 3; ref_index++)
    cm->frame_refs[ref_index].idx = INT_MAX;
}

int vp9_receive_compressed_data(VP9Decoder *pbi,
                                size_t size, const uint8_t **psource) {
  VP9_COMMON *const cm = &pbi->common;
  BufferPool *const pool = cm->buffer_pool;
  RefCntBuffer *const frame_bufs = cm->buffer_pool->frame_bufs;
  const uint8_t *source = *psource;
  int retcode = 0;

  cm->error.error_code = VPX_CODEC_OK;

  if (size == 0) {
    // This is used to signal that we are missing frames.
    // We do not know if the missing frame(s) was supposed to update
    // any of the reference buffers, but we act conservative and
    // mark only the last buffer as corrupted.
    //
    // TODO(jkoleszar): Error concealment is undefined and non-normative
    // at this point, but if it becomes so, [0] may not always be the correct
    // thing to do here.
    if (cm->frame_refs[0].idx != INT_MAX)
      cm->frame_refs[0].buf->corrupted = 1;
  }

  // Check if the previous frame was a frame without any references to it.
  // Release frame buffer if not decoding in frame parallel mode.
  if (!pbi->frame_parallel_decode && cm->new_fb_idx >= 0
      && frame_bufs[cm->new_fb_idx].ref_count == 0)
    pool->release_fb_cb(pool->cb_priv,
                        &frame_bufs[cm->new_fb_idx].raw_frame_buffer);
  cm->new_fb_idx = get_free_fb(cm);

  if (pbi->frame_parallel_decode) {
    VP9Worker *worker = pbi->owner_frame_worker;
    FrameWorkerData *const worker_data = worker->data1;
    pbi->cur_buf = &pool->frame_bufs[cm->new_fb_idx];
    pool->frame_bufs[cm->new_fb_idx].owner_worker_id = worker_data->worker_id;

    // Reset the decoding progress.
    pbi->cur_buf->row = -1;
    pbi->cur_buf->col = -1;
  }

  if (setjmp(cm->error.jmp)) {
    cm->error.setjmp = 0;

    // We do not know if the missing frame(s) was supposed to update
    // any of the reference buffers, but we act conservative and
    // mark only the last buffer as corrupted.
    //
    // TODO(jkoleszar): Error concealment is undefined and non-normative
    // at this point, but if it becomes so, [0] may not always be the correct
    // thing to do here.
    if (cm->frame_refs[0].idx != INT_MAX && cm->frame_refs[0].buf != NULL)
      cm->frame_refs[0].buf->corrupted = 1;

    if (frame_bufs[cm->new_fb_idx].ref_count > 0)
      --frame_bufs[cm->new_fb_idx].ref_count;

    return -1;
  }

  cm->error.setjmp = 1;

  vp9_decode_frame(pbi, source, source + size, psource);

  swap_frame_buffers(pbi);

  vp9_clear_system_state();

  cm->last_width = cm->width;
  cm->last_height = cm->height;

  if (!cm->show_existing_frame)
    cm->last_show_frame = cm->show_frame;
  if (cm->show_frame) {
    if (!cm->show_existing_frame)
      vp9_swap_mi_and_prev_mi(cm);

    cm->current_video_frame++;
  }

  pbi->ready_for_new_data = 0;

  cm->error.setjmp = 0;
  return retcode;
}

int vp9_get_raw_frame(VP9Decoder *pbi, YV12_BUFFER_CONFIG *sd,
                      vp9_ppflags_t *flags) {
  int ret = -1;
#if !CONFIG_VP9_POSTPROC
  (void)*flags;
#endif

  if (pbi->ready_for_new_data == 1)
    return ret;

  /* no raw frame to show!!! */
  if (pbi->common.show_frame == 0)
    return ret;

  pbi->ready_for_new_data = 1;

#if CONFIG_VP9_POSTPROC
  ret = vp9_post_proc_frame(&pbi->common, sd, flags);
#else
  *sd = *pbi->common.frame_to_show;
  ret = 0;
#endif /*!CONFIG_POSTPROC*/
  vp9_clear_system_state();
  return ret;
}
