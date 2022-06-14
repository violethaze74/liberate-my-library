/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VP10_DECODER_DECODER_H_
#define VP10_DECODER_DECODER_H_

#include "./vpx_config.h"

#include "vpx/vpx_codec.h"
#include "vp10/decoder/bitreader.h"
#include "vpx_scale/yv12config.h"
#include "vpx_util/vpx_thread.h"

#include "vp10/common/thread_common.h"
#include "vp10/common/onyxc_int.h"
#include "vp10/decoder/dthread.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO(hkuang): combine this with TileWorkerData.
typedef struct TileData {
  VP10_COMMON *cm;
  vp10_reader bit_reader;
  DECLARE_ALIGNED(16, MACROBLOCKD, xd);
  /* dqcoeff are shared by all the planes. So planes must be decoded serially */
  DECLARE_ALIGNED(16, tran_low_t, dqcoeff[MAX_TX_SQUARE]);
  DECLARE_ALIGNED(16, uint8_t, color_index_map[2][MAX_SB_SQUARE]);
} TileData;

typedef struct TileWorkerData {
  struct VP10Decoder *pbi;
  vp10_reader bit_reader;
  FRAME_COUNTS counts;
  DECLARE_ALIGNED(16, MACROBLOCKD, xd);
  /* dqcoeff are shared by all the planes. So planes must be decoded serially */
  DECLARE_ALIGNED(16, tran_low_t, dqcoeff[MAX_TX_SQUARE]);
  DECLARE_ALIGNED(16, uint8_t, color_index_map[2][MAX_SB_SQUARE]);
  struct vpx_internal_error_info error_info;
} TileWorkerData;

typedef struct TileBufferDec {
  const uint8_t *data;
  size_t size;
  const uint8_t *raw_data_end;  // The end of the raw tile buffer in the
                                // bit stream.
  int col;  // only used with multi-threaded decoding
} TileBufferDec;

typedef struct VP10Decoder {
  DECLARE_ALIGNED(16, MACROBLOCKD, mb);

  DECLARE_ALIGNED(16, VP10_COMMON, common);

  int ready_for_new_data;

  int refresh_frame_flags;

  // TODO(hkuang): Combine this with cur_buf in macroblockd as they are
  // the same.
  RefCntBuffer *cur_buf;   //  Current decoding frame buffer.

  VPxWorker *frame_worker_owner;   // frame_worker that owns this pbi.
  VPxWorker lf_worker;
  VPxWorker *tile_workers;
  TileWorkerData *tile_worker_data;
  TileInfo *tile_worker_info;
  int num_tile_workers;

  TileData *tile_data;
  int allocated_tiles;

  TileBufferDec tile_buffers[MAX_TILE_ROWS][MAX_TILE_COLS];

  VP10LfSync lf_row_sync;

  vpx_decrypt_cb decrypt_cb;
  void *decrypt_state;

  int max_threads;
  int inv_tile_order;
  int need_resync;  // wait for key/intra-only frame.
  int hold_ref_buf;  // hold the reference buffer.

  int tile_size_bytes;
#if CONFIG_EXT_TILE
  int tile_col_size_bytes;
  int dec_tile_row, dec_tile_col;
#endif  // CONFIG_EXT_TILE
} VP10Decoder;

int vp10_receive_compressed_data(struct VP10Decoder *pbi,
                                size_t size, const uint8_t **dest);

int vp10_get_raw_frame(struct VP10Decoder *pbi, YV12_BUFFER_CONFIG *sd);

int vp10_get_frame_to_show(struct VP10Decoder *pbi, YV12_BUFFER_CONFIG *frame);

vpx_codec_err_t vp10_copy_reference_dec(struct VP10Decoder *pbi,
                                       VPX_REFFRAME ref_frame_flag,
                                       YV12_BUFFER_CONFIG *sd);

vpx_codec_err_t vp10_set_reference_dec(VP10_COMMON *cm,
                                      VPX_REFFRAME ref_frame_flag,
                                      YV12_BUFFER_CONFIG *sd);

static INLINE uint8_t read_marker(vpx_decrypt_cb decrypt_cb,
                                  void *decrypt_state,
                                  const uint8_t *data) {
  if (decrypt_cb) {
    uint8_t marker;
    decrypt_cb(decrypt_state, data, &marker, 1);
    return marker;
  }
  return *data;
}

// This function is exposed for use in tests, as well as the inlined function
// "read_marker".
vpx_codec_err_t vp10_parse_superframe_index(const uint8_t *data,
                                           size_t data_sz,
                                           uint32_t sizes[8], int *count,
                                           vpx_decrypt_cb decrypt_cb,
                                           void *decrypt_state);

struct VP10Decoder *vp10_decoder_create(BufferPool *const pool);

void vp10_decoder_remove(struct VP10Decoder *pbi);

static INLINE void decrease_ref_count(int idx, RefCntBuffer *const frame_bufs,
                                      BufferPool *const pool) {
  if (idx >= 0) {
    --frame_bufs[idx].ref_count;
    // A worker may only get a free framebuffer index when calling get_free_fb.
    // But the private buffer is not set up until finish decoding header.
    // So any error happens during decoding header, the frame_bufs will not
    // have valid priv buffer.
    if (frame_bufs[idx].ref_count == 0 &&
        frame_bufs[idx].raw_frame_buffer.priv) {
      pool->release_fb_cb(pool->cb_priv, &frame_bufs[idx].raw_frame_buffer);
    }
  }
}

#if CONFIG_EXT_REFS
static INLINE int dec_is_ref_frame_buf(VP10Decoder *const pbi,
                                       RefCntBuffer *frame_buf) {
  VP10_COMMON *const cm = &pbi->common;
  int i;
  for (i = 0; i < INTER_REFS_PER_FRAME; ++i) {
    RefBuffer *const ref_frame = &cm->frame_refs[i];
    if (ref_frame->idx == INVALID_IDX) continue;
    if (frame_buf == &cm->buffer_pool->frame_bufs[ref_frame->idx])
      break;
  }
  return (i < INTER_REFS_PER_FRAME);
}
#endif  // CONFIG_EXT_REFS

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VP10_DECODER_DECODER_H_
