/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./ivfenc.h"

#include "./tools_common.h"
#include "vpx/vpx_encoder.h"
#include "vpx_ports/mem_ops.h"

void ivf_write_file_header(FILE *outfile,
                           const struct vpx_codec_enc_cfg *cfg,
                           unsigned int fourcc,
                           int frame_cnt) {
  char header[32];

  if (cfg->g_pass != VPX_RC_ONE_PASS && cfg->g_pass != VPX_RC_LAST_PASS)
    return;

  header[0] = 'D';
  header[1] = 'K';
  header[2] = 'I';
  header[3] = 'F';
  mem_put_le16(header + 4,  0);                 /* version */
  mem_put_le16(header + 6,  32);                /* headersize */
  mem_put_le32(header + 8,  fourcc);            /* four CC */
  mem_put_le16(header + 12, cfg->g_w);          /* width */
  mem_put_le16(header + 14, cfg->g_h);          /* height */
  mem_put_le32(header + 16, cfg->g_timebase.den); /* rate */
  mem_put_le32(header + 20, cfg->g_timebase.num); /* scale */
  mem_put_le32(header + 24, frame_cnt);         /* length */
  mem_put_le32(header + 28, 0);                 /* unused */

  (void) fwrite(header, 1, 32, outfile);
}

void ivf_write_frame_header(FILE *outfile, const struct vpx_codec_cx_pkt *pkt) {
  char header[12];
  vpx_codec_pts_t pts;

  if (pkt->kind != VPX_CODEC_CX_FRAME_PKT)
    return;

  pts = pkt->data.frame.pts;
  mem_put_le32(header, (int)pkt->data.frame.sz);
  mem_put_le32(header + 4, pts & 0xFFFFFFFF);
  mem_put_le32(header + 8, pts >> 32);

  (void) fwrite(header, 1, 12, outfile);
}

void ivf_write_frame_size(FILE *outfile, size_t size) {
  char header[4];
  mem_put_le32(header, (int)size);
  (void) fwrite(header, 1, 4, outfile);
}
