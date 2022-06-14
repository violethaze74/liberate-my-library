/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * This is an example demonstrating how to implement a multi-layer
 * VP9 encoding scheme based on spatial scalability for video applications
 * that benefit from a scalable bitstream.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "./args.h"
#include "./ivfenc.h"
#include "./tools_common.h"
#include "vpx/svc_context.h"
#include "vpx/vp8cx.h"
#include "vpx/vpx_encoder.h"

static const struct arg_enum_list encoding_mode_enum[] = {
  {"i", INTER_LAYER_PREDICTION_I},
  {"alt-ip", ALT_INTER_LAYER_PREDICTION_IP},
  {"ip", INTER_LAYER_PREDICTION_IP},
  {"gf", USE_GOLDEN_FRAME},
  {NULL, 0}
};

static const arg_def_t encoding_mode_arg = ARG_DEF_ENUM(
    "m", "encoding-mode", 1, "Encoding mode algorithm", encoding_mode_enum);
static const arg_def_t skip_frames_arg =
    ARG_DEF("s", "skip-frames", 1, "input frames to skip");
static const arg_def_t frames_arg =
    ARG_DEF("f", "frames", 1, "number of frames to encode");
static const arg_def_t width_arg = ARG_DEF("w", "width", 1, "source width");
static const arg_def_t height_arg = ARG_DEF("h", "height", 1, "source height");
static const arg_def_t timebase_arg =
    ARG_DEF("t", "timebase", 1, "timebase (num/den)");
static const arg_def_t bitrate_arg = ARG_DEF(
    "b", "target-bitrate", 1, "encoding bitrate, in kilobits per second");
static const arg_def_t layers_arg =
    ARG_DEF("l", "layers", 1, "number of SVC layers");
static const arg_def_t kf_dist_arg =
    ARG_DEF("k", "kf-dist", 1, "number of frames between keyframes");
static const arg_def_t scale_factors_arg =
    ARG_DEF("r", "scale-factors", 1, "scale factors (lowest to highest layer)");
static const arg_def_t quantizers_arg =
    ARG_DEF("q", "quantizers", 1, "quantizers (lowest to highest layer)");

static const arg_def_t *svc_args[] = {
  &encoding_mode_arg, &frames_arg,        &width_arg,       &height_arg,
  &timebase_arg,      &bitrate_arg,       &skip_frames_arg, &layers_arg,
  &kf_dist_arg,       &scale_factors_arg, &quantizers_arg,  NULL
};

static const SVC_ENCODING_MODE default_encoding_mode =
    INTER_LAYER_PREDICTION_IP;
static const uint32_t default_frames_to_skip = 0;
static const uint32_t default_frames_to_code = 60 * 60;
static const uint32_t default_width = 1920;
static const uint32_t default_height = 1080;
static const uint32_t default_timebase_num = 1;
static const uint32_t default_timebase_den = 60;
static const uint32_t default_bitrate = 1000;
static const uint32_t default_spatial_layers = 5;
static const uint32_t default_kf_dist = 100;

typedef struct {
  char *output_filename;
  uint32_t frames_to_code;
  uint32_t frames_to_skip;
  struct VpxInputContext input_ctx;
} AppInput;

void usage_exit(const char *exec_name) {
  fprintf(stderr, "Usage: %s <options> input_filename output_filename\n",
          exec_name);
  fprintf(stderr, "Options:\n");
  arg_show_usage(stderr, svc_args);
  exit(EXIT_FAILURE);
}

static void die_codec(vpx_codec_ctx_t *ctx, const char *s) {
  const char *detail = vpx_codec_error_detail(ctx);

  printf("%s: %s\n", s, vpx_codec_error(ctx));
  if (detail) printf("    %s\n", detail);
  exit(EXIT_FAILURE);
}

static int create_dummy_frame(vpx_image_t *img) {
  const size_t buf_size = img->w * img->h * 3 / 2;
  memset(img->planes[0], 129, buf_size);
  return 1;
}

static void parse_command_line(int argc, const char **argv_,
                               AppInput *app_input, SvcContext *svc_ctx,
                               vpx_codec_enc_cfg_t *enc_cfg) {
  struct arg arg;
  char **argv, **argi, **argj;
  vpx_codec_err_t res;

  // initialize SvcContext with parameters that will be passed to vpx_svc_init
  svc_ctx->log_level = SVC_LOG_DEBUG;
  svc_ctx->spatial_layers = default_spatial_layers;
  svc_ctx->encoding_mode = default_encoding_mode;

  // start with default encoder configuration
  res = vpx_codec_enc_config_default(vpx_codec_vp9_cx(), enc_cfg, 0);
  if (res) {
    die("Failed to get config: %s\n", vpx_codec_err_to_string(res));
  }
  // update enc_cfg with app default values
  enc_cfg->g_w = default_width;
  enc_cfg->g_h = default_height;
  enc_cfg->g_timebase.num = default_timebase_num;
  enc_cfg->g_timebase.den = default_timebase_den;
  enc_cfg->rc_target_bitrate = default_bitrate;
  enc_cfg->kf_min_dist = default_kf_dist;
  enc_cfg->kf_max_dist = default_kf_dist;

  // initialize AppInput with default values
  app_input->frames_to_code = default_frames_to_code;
  app_input->frames_to_skip = default_frames_to_skip;

  // process command line options
  argv = argv_dup(argc - 1, argv_ + 1);
  for (argi = argj = argv; (*argj = *argi); argi += arg.argv_step) {
    arg.argv_step = 1;

    if (arg_match(&arg, &encoding_mode_arg, argi)) {
      svc_ctx->encoding_mode = arg_parse_enum_or_int(&arg);
    } else if (arg_match(&arg, &frames_arg, argi)) {
      app_input->frames_to_code = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &width_arg, argi)) {
      enc_cfg->g_w = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &height_arg, argi)) {
      enc_cfg->g_h = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &timebase_arg, argi)) {
      enc_cfg->g_timebase = arg_parse_rational(&arg);
    } else if (arg_match(&arg, &bitrate_arg, argi)) {
      enc_cfg->rc_target_bitrate = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &skip_frames_arg, argi)) {
      app_input->frames_to_skip = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &layers_arg, argi)) {
      svc_ctx->spatial_layers = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &kf_dist_arg, argi)) {
      enc_cfg->kf_min_dist = arg_parse_uint(&arg);
      enc_cfg->kf_max_dist = enc_cfg->kf_min_dist;
    } else if (arg_match(&arg, &scale_factors_arg, argi)) {
      vpx_svc_set_scale_factors(svc_ctx, arg.val);
    } else if (arg_match(&arg, &quantizers_arg, argi)) {
      vpx_svc_set_quantizers(svc_ctx, arg.val);
    } else {
      ++argj;
    }
  }

  // Check for unrecognized options
  for (argi = argv; *argi; ++argi)
    if (argi[0][0] == '-' && strlen(argi[0]) > 1)
      die("Error: Unrecognized option %s\n", *argi);

  if (argv[0] == NULL || argv[1] == 0) {
    usage_exit(argv_[0]);
  }
  app_input->input_ctx.filename = argv[0];
  app_input->output_filename = argv[1];
  free(argv);

  if (enc_cfg->g_w < 16 || enc_cfg->g_w % 2 || enc_cfg->g_h < 16 ||
      enc_cfg->g_h % 2)
    die("Invalid resolution: %d x %d\n", enc_cfg->g_w, enc_cfg->g_h);

  printf(
      "Codec %s\nframes: %d, skip: %d\n"
      "mode: %d, layers: %d\n"
      "width %d, height: %d,\n"
      "num: %d, den: %d, bitrate: %d,\n"
      "gop size: %d\n",
      vpx_codec_iface_name(vpx_codec_vp9_cx()), app_input->frames_to_code,
      app_input->frames_to_skip, svc_ctx->encoding_mode,
      svc_ctx->spatial_layers, enc_cfg->g_w, enc_cfg->g_h,
      enc_cfg->g_timebase.num, enc_cfg->g_timebase.den,
      enc_cfg->rc_target_bitrate, enc_cfg->kf_max_dist);
}

int main(int argc, const char **argv) {
  AppInput app_input = {0};
  FILE *outfile;
  vpx_codec_ctx_t codec;
  vpx_codec_enc_cfg_t enc_cfg;
  SvcContext svc_ctx;
  uint32_t i;
  uint32_t frame_cnt = 0;
  vpx_image_t raw;
  vpx_codec_err_t res;
  int pts = 0;            /* PTS starts at 0 */
  int frame_duration = 1; /* 1 timebase tick per frame */
  vpx_codec_cx_pkt_t packet = {0};
  packet.kind = VPX_CODEC_CX_FRAME_PKT;

  memset(&svc_ctx, 0, sizeof(svc_ctx));
  svc_ctx.log_print = 1;
  parse_command_line(argc, argv, &app_input, &svc_ctx, &enc_cfg);

  // Allocate image buffer
  if (!vpx_img_alloc(&raw, VPX_IMG_FMT_I420, enc_cfg.g_w, enc_cfg.g_h, 32))
    die("Failed to allocate image %dx%d\n", enc_cfg.g_w, enc_cfg.g_h);

  if (!(app_input.input_ctx.file = fopen(app_input.input_ctx.filename, "rb")))
    die("Failed to open %s for reading\n", app_input.input_ctx.filename);

  if (!(outfile = fopen(app_input.output_filename, "wb")))
    die("Failed to open %s for writing\n", app_input.output_filename);

  // Initialize codec
  if (vpx_svc_init(&svc_ctx, &codec, vpx_codec_vp9_cx(), &enc_cfg) !=
      VPX_CODEC_OK)
    die("Failed to initialize encoder\n");

  ivf_write_file_header(outfile, &enc_cfg, VP9_FOURCC, 0);

  // skip initial frames
  for (i = 0; i < app_input.frames_to_skip; ++i) {
    read_yuv_frame(&app_input.input_ctx, &raw);
  }

  // Encode frames
  while (frame_cnt < app_input.frames_to_code) {
    if (read_yuv_frame(&app_input.input_ctx, &raw)) break;

    res = vpx_svc_encode(&svc_ctx, &codec, &raw, pts, frame_duration,
                         VPX_DL_REALTIME);
    printf("%s", vpx_svc_get_message(&svc_ctx));
    if (res != VPX_CODEC_OK) {
      die_codec(&codec, "Failed to encode frame");
    }
    if (vpx_svc_get_frame_size(&svc_ctx) > 0) {
      packet.data.frame.pts = pts;
      packet.data.frame.sz = vpx_svc_get_frame_size(&svc_ctx);
      ivf_write_frame_header(outfile, &packet);
      (void)fwrite(vpx_svc_get_buffer(&svc_ctx), 1,
                   vpx_svc_get_frame_size(&svc_ctx), outfile);
    }
    ++frame_cnt;
    pts += frame_duration;
  }

  printf("Processed %d frames\n", frame_cnt);

  fclose(app_input.input_ctx.file);
  if (vpx_codec_destroy(&codec)) die_codec(&codec, "Failed to destroy codec");

  // rewrite the output file headers with the actual frame count
  if (!fseek(outfile, 0, SEEK_SET)) {
    ivf_write_file_header(outfile, &enc_cfg, VP9_FOURCC, frame_cnt);
  }
  fclose(outfile);
  vpx_img_free(&raw);

  // display average size, psnr
  printf("%s", vpx_svc_dump_statistics(&svc_ctx));

  vpx_svc_release(&svc_ctx);

  return EXIT_SUCCESS;
}
