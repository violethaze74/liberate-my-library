/*
 *  Copyright (c) 2014 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#include "./vp9_rtcd.h"

#include "vpx_mem/vpx_mem.h"

#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_mvref_common.h"
#include "vp9/common/vp9_reconinter.h"
#include "vp9/common/vp9_reconintra.h"

#include "vp9/encoder/vp9_encoder.h"
#include "vp9/encoder/vp9_pickmode.h"
#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/encoder/vp9_rdopt.h"

static int mv_refs_rt(const VP9_COMMON *cm, const MACROBLOCKD *xd,
                       const TileInfo *const tile,
                       MODE_INFO *mi, MV_REFERENCE_FRAME ref_frame,
                       int_mv *mv_ref_list,
                       int mi_row, int mi_col) {
  const int *ref_sign_bias = cm->ref_frame_sign_bias;
  int i, refmv_count = 0;

  const POSITION *const mv_ref_search = mv_ref_blocks[mi->mbmi.sb_type];

  int different_ref_found = 0;
  int context_counter = 0;
  int const_motion = 0;

  // Blank the reference vector list
  vpx_memset(mv_ref_list, 0, sizeof(*mv_ref_list) * MAX_MV_REF_CANDIDATES);

  // The nearest 2 blocks are treated differently
  // if the size < 8x8 we get the mv from the bmi substructure,
  // and we also need to keep a mode count.
  for (i = 0; i < 2; ++i) {
    const POSITION *const mv_ref = &mv_ref_search[i];
    if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
      const MODE_INFO *const candidate_mi = xd->mi[mv_ref->col + mv_ref->row *
                                                   xd->mi_stride];
      const MB_MODE_INFO *const candidate = &candidate_mi->mbmi;
      // Keep counts for entropy encoding.
      context_counter += mode_2_counter[candidate->mode];
      different_ref_found = 1;

      if (candidate->ref_frame[0] == ref_frame)
        ADD_MV_REF_LIST(get_sub_block_mv(candidate_mi, 0, mv_ref->col, -1));
    }
  }

  const_motion = 1;

  // Check the rest of the neighbors in much the same way
  // as before except we don't need to keep track of sub blocks or
  // mode counts.
  for (; i < MVREF_NEIGHBOURS && !refmv_count; ++i) {
    const POSITION *const mv_ref = &mv_ref_search[i];
    if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
      const MB_MODE_INFO *const candidate = &xd->mi[mv_ref->col + mv_ref->row *
                                                    xd->mi_stride]->mbmi;
      different_ref_found = 1;

      if (candidate->ref_frame[0] == ref_frame)
        ADD_MV_REF_LIST(candidate->mv[0]);
    }
  }

  // Since we couldn't find 2 mvs from the same reference frame
  // go back through the neighbors and find motion vectors from
  // different reference frames.
  if (different_ref_found && !refmv_count) {
    for (i = 0; i < MVREF_NEIGHBOURS; ++i) {
      const POSITION *mv_ref = &mv_ref_search[i];
      if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
        const MB_MODE_INFO *const candidate = &xd->mi[mv_ref->col + mv_ref->row
                                              * xd->mi_stride]->mbmi;

        // If the candidate is INTRA we don't want to consider its mv.
        IF_DIFF_REF_FRAME_ADD_MV(candidate);
      }
    }
  }

 Done:

  mi->mbmi.mode_context[ref_frame] = counter_to_context[context_counter];

  // Clamp vectors
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i)
    clamp_mv_ref(&mv_ref_list[i].as_mv, xd);

  return const_motion;
}

static void full_pixel_motion_search(VP9_COMP *cpi, MACROBLOCK *x,
                                    BLOCK_SIZE bsize, int mi_row, int mi_col,
                                    int_mv *tmp_mv, int *rate_mv) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = &xd->mi[0]->mbmi;
  struct buf_2d backup_yv12[MAX_MB_PLANE] = {{0, 0}};
  int step_param;
  int sadpb = x->sadperbit16;
  MV mvp_full;
  int ref = mbmi->ref_frame[0];
  const MV ref_mv = mbmi->ref_mvs[ref][0].as_mv;
  int i;

  int tmp_col_min = x->mv_col_min;
  int tmp_col_max = x->mv_col_max;
  int tmp_row_min = x->mv_row_min;
  int tmp_row_max = x->mv_row_max;

  const YV12_BUFFER_CONFIG *scaled_ref_frame = vp9_get_scaled_ref_frame(cpi,
                                                                        ref);
  if (scaled_ref_frame) {
    int i;
    // Swap out the reference frame for a version that's been scaled to
    // match the resolution of the current frame, allowing the existing
    // motion search code to be used without additional modifications.
    for (i = 0; i < MAX_MB_PLANE; i++)
      backup_yv12[i] = xd->plane[i].pre[0];

    vp9_setup_pre_planes(xd, 0, scaled_ref_frame, mi_row, mi_col, NULL);
  }

  vp9_set_mv_search_range(x, &ref_mv);

  // TODO(jingning) exploiting adaptive motion search control in non-RD
  // mode decision too.
  step_param = 6;

  for (i = LAST_FRAME; i <= LAST_FRAME && cpi->common.show_frame; ++i) {
    if ((x->pred_mv_sad[ref] >> 3) > x->pred_mv_sad[i]) {
      tmp_mv->as_int = INVALID_MV;

      if (scaled_ref_frame) {
        int i;
        for (i = 0; i < MAX_MB_PLANE; i++)
          xd->plane[i].pre[0] = backup_yv12[i];
      }
      return;
    }
  }
  assert(x->mv_best_ref_index[ref] <= 2);
  if (x->mv_best_ref_index[ref] < 2)
    mvp_full = mbmi->ref_mvs[ref][x->mv_best_ref_index[ref]].as_mv;
  else
    mvp_full = x->pred_mv[ref];

  mvp_full.col >>= 3;
  mvp_full.row >>= 3;

  vp9_full_pixel_search(cpi, x, bsize, &mvp_full, step_param, sadpb, &ref_mv,
                        &tmp_mv->as_mv, INT_MAX, 0);

  x->mv_col_min = tmp_col_min;
  x->mv_col_max = tmp_col_max;
  x->mv_row_min = tmp_row_min;
  x->mv_row_max = tmp_row_max;

  if (scaled_ref_frame) {
    int i;
    for (i = 0; i < MAX_MB_PLANE; i++)
      xd->plane[i].pre[0] = backup_yv12[i];
  }

  // calculate the bit cost on motion vector
  mvp_full.row = tmp_mv->as_mv.row * 8;
  mvp_full.col = tmp_mv->as_mv.col * 8;
  *rate_mv = vp9_mv_bit_cost(&mvp_full, &ref_mv,
                             x->nmvjointcost, x->mvcost, MV_COST_WEIGHT);
}

static void sub_pixel_motion_search(VP9_COMP *cpi, MACROBLOCK *x,
                                    BLOCK_SIZE bsize, int mi_row, int mi_col,
                                    MV *tmp_mv) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = &xd->mi[0]->mbmi;
  struct buf_2d backup_yv12[MAX_MB_PLANE] = {{0, 0}};
  int ref = mbmi->ref_frame[0];
  MV ref_mv = mbmi->ref_mvs[ref][0].as_mv;
  int dis;

  const YV12_BUFFER_CONFIG *scaled_ref_frame = vp9_get_scaled_ref_frame(cpi,
                                                                        ref);
  if (scaled_ref_frame) {
    int i;
    // Swap out the reference frame for a version that's been scaled to
    // match the resolution of the current frame, allowing the existing
    // motion search code to be used without additional modifications.
    for (i = 0; i < MAX_MB_PLANE; i++)
      backup_yv12[i] = xd->plane[i].pre[0];

    vp9_setup_pre_planes(xd, 0, scaled_ref_frame, mi_row, mi_col, NULL);
  }

  cpi->find_fractional_mv_step(x, tmp_mv, &ref_mv,
                               cpi->common.allow_high_precision_mv,
                               x->errorperbit,
                               &cpi->fn_ptr[bsize],
                               cpi->sf.mv.subpel_force_stop,
                               cpi->sf.mv.subpel_iters_per_step,
                               x->nmvjointcost, x->mvcost,
                               &dis, &x->pred_sse[ref]);

  if (scaled_ref_frame) {
    int i;
    for (i = 0; i < MAX_MB_PLANE; i++)
      xd->plane[i].pre[0] = backup_yv12[i];
  }

  x->pred_mv[ref] = *tmp_mv;
}

static void model_rd_for_sb_y(VP9_COMP *cpi, BLOCK_SIZE bsize,
                              MACROBLOCK *x, MACROBLOCKD *xd,
                              int *out_rate_sum, int64_t *out_dist_sum,
                              unsigned int *var_y, unsigned int *sse_y) {
  // Note our transform coeffs are 8 times an orthogonal transform.
  // Hence quantizer step is also 8 times. To get effective quantizer
  // we need to divide by 8 before sending to modeling function.
  unsigned int sse;
  int rate;
  int64_t dist;
  struct macroblock_plane *const p = &x->plane[0];
  struct macroblockd_plane *const pd = &xd->plane[0];
  const uint32_t dc_quant = pd->dequant[0];
  const uint32_t ac_quant = pd->dequant[1];
  unsigned int var = cpi->fn_ptr[bsize].vf(p->src.buf, p->src.stride,
                                           pd->dst.buf, pd->dst.stride, &sse);
  *var_y = var;
  *sse_y = sse;

  if (sse < dc_quant * dc_quant >> 6)
    x->skip_txfm = 1;
  else if (var < ac_quant * ac_quant >> 6)
    x->skip_txfm = 2;
  else
    x->skip_txfm = 0;

  vp9_model_rd_from_var_lapndz(sse - var, 1 << num_pels_log2_lookup[bsize],
                               dc_quant >> 3, &rate, &dist);
  *out_rate_sum = rate >> 1;
  *out_dist_sum = dist << 3;

  vp9_model_rd_from_var_lapndz(var, 1 << num_pels_log2_lookup[bsize],
                               ac_quant >> 3, &rate, &dist);
  *out_rate_sum += rate;
  *out_dist_sum += dist << 4;
}

static int get_pred_buffer(PRED_BUFFER *p, int len) {
  int i;

  for (i = 0; i < len; i++) {
    if (!p[i].in_use) {
      p[i].in_use = 1;
      return i;
    }
  }
  return -1;
}

static void free_pred_buffer(PRED_BUFFER *p) {
  p->in_use = 0;
}

// TODO(jingning) placeholder for inter-frame non-RD mode decision.
// this needs various further optimizations. to be continued..
int64_t vp9_pick_inter_mode(VP9_COMP *cpi, MACROBLOCK *x,
                            const TileInfo *const tile,
                            int mi_row, int mi_col,
                            int *returnrate,
                            int64_t *returndistortion,
                            BLOCK_SIZE bsize) {
  MACROBLOCKD *xd = &x->e_mbd;
  MB_MODE_INFO *mbmi = &xd->mi[0]->mbmi;
  struct macroblock_plane *const p = &x->plane[0];
  struct macroblockd_plane *const pd = &xd->plane[0];
  PREDICTION_MODE this_mode, best_mode = ZEROMV;
  MV_REFERENCE_FRAME ref_frame, best_ref_frame = LAST_FRAME;
  INTERP_FILTER best_pred_filter = EIGHTTAP;
  int_mv frame_mv[MB_MODE_COUNT][MAX_REF_FRAMES];
  struct buf_2d yv12_mb[4][MAX_MB_PLANE];
  static const int flag_list[4] = { 0, VP9_LAST_FLAG, VP9_GOLD_FLAG,
                                    VP9_ALT_FLAG };
  int64_t best_rd = INT64_MAX;
  int64_t this_rd = INT64_MAX;
  int skip_txfm = 0;

  int rate = INT_MAX;
  int64_t dist = INT64_MAX;
  // var_y and sse_y are saved to be used in skipping checking
  unsigned int var_y = UINT_MAX;
  unsigned int sse_y = UINT_MAX;

  VP9_COMMON *cm = &cpi->common;
  int intra_cost_penalty = 20 * vp9_dc_quant(cm->base_qindex, cm->y_dc_delta_q);

  const int64_t inter_mode_thresh = RDCOST(x->rdmult, x->rddiv,
                                           intra_cost_penalty, 0);
  const int64_t intra_mode_cost = 50;

  unsigned char segment_id = mbmi->segment_id;
  const int *const rd_threshes = cpi->rd.threshes[segment_id][bsize];
  const int *const rd_thresh_freq_fact = cpi->rd.thresh_freq_fact[bsize];
  // Mode index conversion form THR_MODES to PREDICTION_MODE for a ref frame.
  int mode_idx[MB_MODE_COUNT] = {0};
  INTERP_FILTER filter_ref = SWITCHABLE;
  int bsl = mi_width_log2_lookup[bsize];
  const int pred_filter_search = (((mi_row + mi_col) >> bsl) +
                                      get_chessboard_index(cm)) % 2;
  int const_motion[MAX_REF_FRAMES] = { 0 };

  // For speed 6, the result of interp filter is reused later in actual encoding
  // process.
  int bh = num_4x4_blocks_high_lookup[bsize] << 2;
  int bw = num_4x4_blocks_wide_lookup[bsize] << 2;
  int pixels_in_block = bh * bw;
  // tmp[3] points to dst buffer, and the other 3 point to allocated buffers.
  PRED_BUFFER tmp[4];
  DECLARE_ALIGNED_ARRAY(16, uint8_t, pred_buf, 3 * 64 * 64);
  struct buf_2d orig_dst = pd->dst;
  PRED_BUFFER *best_pred = NULL;
  PRED_BUFFER *this_mode_pred = NULL;
  int i;

  if (cpi->sf.reuse_inter_pred_sby) {
    for (i = 0; i < 3; i++) {
      tmp[i].data = &pred_buf[pixels_in_block * i];
      tmp[i].stride = bw;
      tmp[i].in_use = 0;
    }

    tmp[3].data = pd->dst.buf;
    tmp[3].stride = pd->dst.stride;
    tmp[3].in_use = 0;
  }

  x->skip_encode = cpi->sf.skip_encode_frame && x->q_index < QIDX_SKIP_THRESH;

  x->skip = 0;

  // initialize mode decisions
  *returnrate = INT_MAX;
  *returndistortion = INT64_MAX;
  vpx_memset(mbmi, 0, sizeof(MB_MODE_INFO));
  mbmi->sb_type = bsize;
  mbmi->ref_frame[0] = NONE;
  mbmi->ref_frame[1] = NONE;
  mbmi->tx_size = MIN(max_txsize_lookup[bsize],
                      tx_mode_to_biggest_tx_size[cm->tx_mode]);
  mbmi->interp_filter = cm->interp_filter == SWITCHABLE ?
                        EIGHTTAP : cm->interp_filter;
  mbmi->skip = 0;
  mbmi->segment_id = segment_id;

  for (ref_frame = LAST_FRAME; ref_frame <= LAST_FRAME ; ++ref_frame) {
    x->pred_mv_sad[ref_frame] = INT_MAX;
    if (cpi->ref_frame_flags & flag_list[ref_frame]) {
      const YV12_BUFFER_CONFIG *yv12 = get_ref_frame_buffer(cpi, ref_frame);
      int_mv *const candidates = mbmi->ref_mvs[ref_frame];
      const struct scale_factors *const sf = &cm->frame_refs[ref_frame - 1].sf;
      vp9_setup_pred_block(xd, yv12_mb[ref_frame], yv12, mi_row, mi_col,
                           sf, sf);

      if (cm->coding_use_prev_mi)
        vp9_find_mv_refs(cm, xd, tile, xd->mi[0], ref_frame,
                         candidates, mi_row, mi_col);
      else
        const_motion[ref_frame] = mv_refs_rt(cm, xd, tile, xd->mi[0],
                                             ref_frame, candidates,
                                             mi_row, mi_col);

      vp9_find_best_ref_mvs(xd, cm->allow_high_precision_mv, candidates,
                            &frame_mv[NEARESTMV][ref_frame],
                            &frame_mv[NEARMV][ref_frame]);

      if (!vp9_is_scaled(sf) && bsize >= BLOCK_8X8)
        vp9_mv_pred(cpi, x, yv12_mb[ref_frame][0].buf, yv12->y_stride,
                    ref_frame, bsize);
    }
    frame_mv[NEWMV][ref_frame].as_int = INVALID_MV;
    frame_mv[ZEROMV][ref_frame].as_int = 0;
  }

  if (xd->up_available)
    filter_ref = xd->mi[-xd->mi_stride]->mbmi.interp_filter;
  else if (xd->left_available)
    filter_ref = xd->mi[-1]->mbmi.interp_filter;

  for (ref_frame = LAST_FRAME; ref_frame <= LAST_FRAME ; ++ref_frame) {
    if (!(cpi->ref_frame_flags & flag_list[ref_frame]))
      continue;

    // Select prediction reference frames.
    xd->plane[0].pre[0] = yv12_mb[ref_frame][0];

    clamp_mv2(&frame_mv[NEARESTMV][ref_frame].as_mv, xd);
    clamp_mv2(&frame_mv[NEARMV][ref_frame].as_mv, xd);

    mbmi->ref_frame[0] = ref_frame;

    // Set conversion index for LAST_FRAME.
    if (ref_frame == LAST_FRAME) {
      mode_idx[NEARESTMV] = THR_NEARESTMV;   // LAST_FRAME, NEARESTMV
      mode_idx[NEARMV] = THR_NEARMV;         // LAST_FRAME, NEARMV
      mode_idx[ZEROMV] = THR_ZEROMV;         // LAST_FRAME, ZEROMV
      mode_idx[NEWMV] = THR_NEWMV;           // LAST_FRAME, NEWMV
    }

    for (this_mode = NEARESTMV; this_mode <= NEWMV; ++this_mode) {
      int rate_mv = 0;

      if (const_motion[ref_frame] &&
          (this_mode == NEARMV || this_mode == ZEROMV))
        continue;

      if (!(cpi->sf.inter_mode_mask[bsize] & (1 << this_mode)))
        continue;

      if (rd_less_than_thresh(best_rd, rd_threshes[mode_idx[this_mode]],
                              rd_thresh_freq_fact[this_mode]))
        continue;

      if (this_mode == NEWMV) {
        int rate_mode = 0;
        if (this_rd < (int64_t)(1 << num_pels_log2_lookup[bsize]))
          continue;

        full_pixel_motion_search(cpi, x, bsize, mi_row, mi_col,
                                 &frame_mv[NEWMV][ref_frame], &rate_mv);

        if (frame_mv[NEWMV][ref_frame].as_int == INVALID_MV)
          continue;

        rate_mode = cpi->inter_mode_cost[mbmi->mode_context[ref_frame]]
                                        [INTER_OFFSET(this_mode)];
        if (RDCOST(x->rdmult, x->rddiv, rate_mv + rate_mode, 0) > best_rd)
          continue;

        sub_pixel_motion_search(cpi, x, bsize, mi_row, mi_col,
                                &frame_mv[NEWMV][ref_frame].as_mv);
      }

      if (this_mode != NEARESTMV)
        if (frame_mv[this_mode][ref_frame].as_int ==
            frame_mv[NEARESTMV][ref_frame].as_int)
          continue;

      mbmi->mode = this_mode;
      mbmi->mv[0].as_int = frame_mv[this_mode][ref_frame].as_int;

      // Search for the best prediction filter type, when the resulting
      // motion vector is at sub-pixel accuracy level for luma component, i.e.,
      // the last three bits are all zeros.
      if (cpi->sf.reuse_inter_pred_sby) {
        if (this_mode == NEARESTMV) {
          this_mode_pred = &tmp[3];
        } else {
          this_mode_pred = &tmp[get_pred_buffer(tmp, 3)];
          pd->dst.buf = this_mode_pred->data;
          pd->dst.stride = bw;
        }
      }

      if ((this_mode == NEWMV || filter_ref == SWITCHABLE) &&
          pred_filter_search &&
          ((mbmi->mv[0].as_mv.row & 0x07) != 0 ||
           (mbmi->mv[0].as_mv.col & 0x07) != 0)) {
        int pf_rate[3];
        int64_t pf_dist[3];
        unsigned int pf_var[3];
        unsigned int pf_sse[3];
        int64_t best_cost = INT64_MAX;
        INTERP_FILTER best_filter = SWITCHABLE, filter;
        PRED_BUFFER *current_pred = this_mode_pred;

        for (filter = EIGHTTAP; filter <= EIGHTTAP_SHARP; ++filter) {
          int64_t cost;
          mbmi->interp_filter = filter;
          vp9_build_inter_predictors_sby(xd, mi_row, mi_col, bsize);
          model_rd_for_sb_y(cpi, bsize, x, xd, &pf_rate[filter],
                            &pf_dist[filter], &pf_var[filter], &pf_sse[filter]);
          cost = RDCOST(x->rdmult, x->rddiv,
                        vp9_get_switchable_rate(cpi) + pf_rate[filter],
                        pf_dist[filter]);
          if (cost < best_cost) {
            best_filter = filter;
            best_cost = cost;
            skip_txfm = x->skip_txfm;

            if (cpi->sf.reuse_inter_pred_sby) {
              if (this_mode_pred != current_pred) {
                free_pred_buffer(this_mode_pred);
                this_mode_pred = current_pred;
              }

              if (filter < EIGHTTAP_SHARP) {
                current_pred = &tmp[get_pred_buffer(tmp, 3)];
                pd->dst.buf = current_pred->data;
                pd->dst.stride = bw;
              }
            }
          }
        }

        if (cpi->sf.reuse_inter_pred_sby && this_mode_pred != current_pred)
          free_pred_buffer(current_pred);

        mbmi->interp_filter = best_filter;
        rate = pf_rate[mbmi->interp_filter];
        dist = pf_dist[mbmi->interp_filter];
        var_y = pf_var[mbmi->interp_filter];
        sse_y = pf_sse[mbmi->interp_filter];
        x->skip_txfm = skip_txfm;
      } else {
        mbmi->interp_filter = (filter_ref == SWITCHABLE) ? EIGHTTAP: filter_ref;
        vp9_build_inter_predictors_sby(xd, mi_row, mi_col, bsize);
        model_rd_for_sb_y(cpi, bsize, x, xd, &rate, &dist, &var_y, &sse_y);
      }

      rate += rate_mv;
      rate += cpi->inter_mode_cost[mbmi->mode_context[ref_frame]]
                                [INTER_OFFSET(this_mode)];
      this_rd = RDCOST(x->rdmult, x->rddiv, rate, dist);

      // Skipping checking: test to see if this block can be reconstructed by
      // prediction only.
      if (cpi->allow_encode_breakout) {
        const BLOCK_SIZE uv_size = get_plane_block_size(bsize, &xd->plane[1]);
        unsigned int var = var_y, sse = sse_y;
        // Skipping threshold for ac.
        unsigned int thresh_ac;
        // Skipping threshold for dc.
        unsigned int thresh_dc;
        if (x->encode_breakout > 0) {
          // Set a maximum for threshold to avoid big PSNR loss in low bit rate
          // case. Use extreme low threshold for static frames to limit
          // skipping.
          const unsigned int max_thresh = 36000;
          // The encode_breakout input
          const unsigned int min_thresh =
              MIN(((unsigned int)x->encode_breakout << 4), max_thresh);

          // Calculate threshold according to dequant value.
          thresh_ac = (xd->plane[0].dequant[1] * xd->plane[0].dequant[1]) / 9;
          thresh_ac = clamp(thresh_ac, min_thresh, max_thresh);

          // Adjust ac threshold according to partition size.
          thresh_ac >>=
              8 - (b_width_log2_lookup[bsize] + b_height_log2_lookup[bsize]);

          thresh_dc = (xd->plane[0].dequant[0] * xd->plane[0].dequant[0] >> 6);
        } else {
          thresh_ac = 0;
          thresh_dc = 0;
        }

        // Y skipping condition checking for ac and dc.
        if (var <= thresh_ac && (sse - var) <= thresh_dc) {
          unsigned int sse_u, sse_v;
          unsigned int var_u, var_v;

          // Skip u v prediction for less calculation, that won't affect
          // result much.
          var_u = cpi->fn_ptr[uv_size].vf(x->plane[1].src.buf,
                                          x->plane[1].src.stride,
                                          xd->plane[1].dst.buf,
                                          xd->plane[1].dst.stride, &sse_u);

          // U skipping condition checking
          if ((var_u * 4 <= thresh_ac) && (sse_u - var_u <= thresh_dc)) {
            var_v = cpi->fn_ptr[uv_size].vf(x->plane[2].src.buf,
                                            x->plane[2].src.stride,
                                            xd->plane[2].dst.buf,
                                            xd->plane[2].dst.stride, &sse_v);

            // V skipping condition checking
            if ((var_v * 4 <= thresh_ac) && (sse_v - var_v <= thresh_dc)) {
              x->skip = 1;

              // The cost of skip bit needs to be added.
              rate = rate_mv;
              rate += cpi->inter_mode_cost[mbmi->mode_context[ref_frame]]
                                           [INTER_OFFSET(this_mode)];

              // More on this part of rate
              // rate += vp9_cost_bit(vp9_get_skip_prob(cm, xd), 1);

              // Scaling factor for SSE from spatial domain to frequency
              // domain is 16. Adjust distortion accordingly.
              // TODO(yunqingwang): In this function, only y-plane dist is
              // calculated.
              dist = (sse << 4);  // + ((sse_u + sse_v) << 4);
              this_rd = RDCOST(x->rdmult, x->rddiv, rate, dist);
              // *disable_skip = 1;
            }
          }
        }
      }

#if CONFIG_DENOISING
    vp9_denoiser_update_frame_stats();
#endif

      if (this_rd < best_rd || x->skip) {
        best_rd = this_rd;
        *returnrate = rate;
        *returndistortion = dist;
        best_mode = this_mode;
        best_pred_filter = mbmi->interp_filter;
        best_ref_frame = ref_frame;
        skip_txfm = x->skip_txfm;

        if (cpi->sf.reuse_inter_pred_sby) {
          if (best_pred != NULL)
            free_pred_buffer(best_pred);

          best_pred = this_mode_pred;
        }
      } else {
        if (cpi->sf.reuse_inter_pred_sby)
          free_pred_buffer(this_mode_pred);
      }

      if (x->skip)
        break;
    }
  }

  // If best prediction is not in dst buf, then copy the prediction block from
  // temp buf to dst buf.
  if (cpi->sf.reuse_inter_pred_sby && best_pred->data != orig_dst.buf) {
    uint8_t *copy_from, *copy_to;

    pd->dst = orig_dst;
    copy_to = pd->dst.buf;

    copy_from = best_pred->data;

    vp9_convolve_copy(copy_from, bw, copy_to, pd->dst.stride, NULL, 0, NULL, 0,
                      bw, bh);
  }

  mbmi->mode = best_mode;
  mbmi->interp_filter = best_pred_filter;
  mbmi->ref_frame[0] = best_ref_frame;
  mbmi->mv[0].as_int = frame_mv[best_mode][best_ref_frame].as_int;
  xd->mi[0]->bmi[0].as_mv[0].as_int = mbmi->mv[0].as_int;
  x->skip_txfm = skip_txfm;

  // Perform intra prediction search, if the best SAD is above a certain
  // threshold.
  if (!x->skip && best_rd > inter_mode_thresh &&
      bsize <= cpi->sf.max_intra_bsize) {
    int i, j;
    const int step   = 1 << mbmi->tx_size;
    const int width  = num_4x4_blocks_wide_lookup[bsize];
    const int height = num_4x4_blocks_high_lookup[bsize];

    int rate2 = 0;
    int64_t dist2 = 0;
    const int dst_stride = pd->dst.stride;
    const int src_stride = p->src.stride;
    int block_idx = 0;

    for (this_mode = DC_PRED; this_mode <= DC_PRED; ++this_mode) {
      if (cpi->sf.reuse_inter_pred_sby) {
        pd->dst.buf = tmp[0].data;
        pd->dst.stride = bw;
      }

      for (j = 0; j < height; j += step) {
        for (i = 0; i < width; i += step) {
          vp9_predict_intra_block(xd, block_idx, b_width_log2(bsize),
                                  mbmi->tx_size, this_mode,
#if CONFIG_FILTERINTRA
                                  0,
#endif
                                  &p->src.buf[4 * (j * dst_stride + i)],
                                  src_stride,
                                  &pd->dst.buf[4 * (j * dst_stride + i)],
                                  dst_stride, i, j, 0);
          model_rd_for_sb_y(cpi, bsize, x, xd, &rate, &dist, &var_y, &sse_y);
          rate2 += rate;
          dist2 += dist;
          ++block_idx;
        }
      }
      rate = rate2;
      dist = dist2;

      rate += cpi->mbmode_cost[this_mode];
      rate += intra_cost_penalty;
      this_rd = RDCOST(x->rdmult, x->rddiv, rate, dist);

      if (cpi->sf.reuse_inter_pred_sby)
        pd->dst = orig_dst;

      if (this_rd + intra_mode_cost < best_rd) {
        best_rd = this_rd;
        *returnrate = rate;
        *returndistortion = dist;
        mbmi->mode = this_mode;
        mbmi->ref_frame[0] = INTRA_FRAME;
        mbmi->uv_mode = this_mode;
        mbmi->mv[0].as_int = INVALID_MV;
      } else {
        x->skip_txfm = skip_txfm;
      }
    }
  }

#if CONFIG_DENOISING
  vp9_denoiser_denoise(&cpi->denoiser, x, mi_row, mi_col, bsize);
#endif

  return INT64_MAX;
}
