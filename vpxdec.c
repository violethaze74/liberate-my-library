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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include "third_party/libyuv/include/libyuv/scale.h"

#include "./args.h"
#include "./ivfdec.h"

#define VPX_CODEC_DISABLE_COMPAT 1
#include "./vpx_config.h"
#include "vpx/vpx_decoder.h"
#include "vpx_ports/vpx_timer.h"

#if CONFIG_VP8_DECODER || CONFIG_VP9_DECODER
#include "vpx/vp8dx.h"
#endif

#include "./md5_utils.h"

#include "./tools_common.h"
#include "./webmdec.h"
#include "./y4menc.h"

static const char *exec_name;

static const struct {
  char const *name;
  const vpx_codec_iface_t *(*iface)(void);
  uint32_t fourcc;
} ifaces[] = {
#if CONFIG_VP8_DECODER
  {"vp8",  vpx_codec_vp8_dx,   VP8_FOURCC},
#endif
#if CONFIG_VP9_DECODER
  {"vp9",  vpx_codec_vp9_dx,   VP9_FOURCC},
#endif
};

struct VpxDecInputContext {
  struct VpxInputContext *vpx_input_ctx;
  struct WebmInputContext *webm_ctx;
};

static const arg_def_t looparg = ARG_DEF(NULL, "loops", 1,
                                          "Number of times to decode the file");
static const arg_def_t codecarg = ARG_DEF(NULL, "codec", 1,
                                          "Codec to use");
static const arg_def_t use_yv12 = ARG_DEF(NULL, "yv12", 0,
                                          "Output raw YV12 frames");
static const arg_def_t use_i420 = ARG_DEF(NULL, "i420", 0,
                                          "Output raw I420 frames");
static const arg_def_t flipuvarg = ARG_DEF(NULL, "flipuv", 0,
                                           "Flip the chroma planes in the output");
static const arg_def_t noblitarg = ARG_DEF(NULL, "noblit", 0,
                                           "Don't process the decoded frames");
static const arg_def_t progressarg = ARG_DEF(NULL, "progress", 0,
                                             "Show progress after each frame decodes");
static const arg_def_t limitarg = ARG_DEF(NULL, "limit", 1,
                                          "Stop decoding after n frames");
static const arg_def_t skiparg = ARG_DEF(NULL, "skip", 1,
                                         "Skip the first n input frames");
static const arg_def_t postprocarg = ARG_DEF(NULL, "postproc", 0,
                                             "Postprocess decoded frames");
static const arg_def_t summaryarg = ARG_DEF(NULL, "summary", 0,
                                            "Show timing summary");
static const arg_def_t outputfile = ARG_DEF("o", "output", 1,
                                            "Output file name pattern (see below)");
static const arg_def_t threadsarg = ARG_DEF("t", "threads", 1,
                                            "Max threads to use");
static const arg_def_t verbosearg = ARG_DEF("v", "verbose", 0,
                                            "Show version string");
static const arg_def_t error_concealment = ARG_DEF(NULL, "error-concealment", 0,
                                                   "Enable decoder error-concealment");
static const arg_def_t scalearg = ARG_DEF("S", "scale", 0,
                                            "Scale output frames uniformly");
static const arg_def_t fb_arg =
    ARG_DEF(NULL, "frame-buffers", 1, "Number of frame buffers to use");
static const arg_def_t fb_lru_arg =
    ARG_DEF(NULL, "frame-buffers-lru", 1, "Turn on/off frame buffer lru");


static const arg_def_t md5arg = ARG_DEF(NULL, "md5", 0,
                                        "Compute the MD5 sum of the decoded frame");

static const arg_def_t *all_args[] = {
  &codecarg, &use_yv12, &use_i420, &flipuvarg, &noblitarg,
  &progressarg, &limitarg, &skiparg, &postprocarg, &summaryarg, &outputfile,
  &threadsarg, &verbosearg, &scalearg, &fb_arg, &fb_lru_arg,
  &md5arg,
  &error_concealment,
  NULL
};

#if CONFIG_VP8_DECODER
static const arg_def_t addnoise_level = ARG_DEF(NULL, "noise-level", 1,
                                                "Enable VP8 postproc add noise");
static const arg_def_t deblock = ARG_DEF(NULL, "deblock", 0,
                                         "Enable VP8 deblocking");
static const arg_def_t demacroblock_level = ARG_DEF(NULL, "demacroblock-level", 1,
                                                    "Enable VP8 demacroblocking, w/ level");
static const arg_def_t pp_debug_info = ARG_DEF(NULL, "pp-debug-info", 1,
                                               "Enable VP8 visible debug info");
static const arg_def_t pp_disp_ref_frame = ARG_DEF(NULL, "pp-dbg-ref-frame", 1,
                                                   "Display only selected reference frame per macro block");
static const arg_def_t pp_disp_mb_modes = ARG_DEF(NULL, "pp-dbg-mb-modes", 1,
                                                  "Display only selected macro block modes");
static const arg_def_t pp_disp_b_modes = ARG_DEF(NULL, "pp-dbg-b-modes", 1,
                                                 "Display only selected block modes");
static const arg_def_t pp_disp_mvs = ARG_DEF(NULL, "pp-dbg-mvs", 1,
                                             "Draw only selected motion vectors");
static const arg_def_t mfqe = ARG_DEF(NULL, "mfqe", 0,
                                      "Enable multiframe quality enhancement");

static const arg_def_t *vp8_pp_args[] = {
  &addnoise_level, &deblock, &demacroblock_level, &pp_debug_info,
  &pp_disp_ref_frame, &pp_disp_mb_modes, &pp_disp_b_modes, &pp_disp_mvs, &mfqe,
  NULL
};
#endif

static int vpx_image_scale(vpx_image_t *src, vpx_image_t *dst,
                           FilterMode mode) {
  assert(src->fmt == VPX_IMG_FMT_I420);
  assert(dst->fmt == VPX_IMG_FMT_I420);
  return I420Scale(src->planes[VPX_PLANE_Y], src->stride[VPX_PLANE_Y],
                   src->planes[VPX_PLANE_U], src->stride[VPX_PLANE_U],
                   src->planes[VPX_PLANE_V], src->stride[VPX_PLANE_V],
                   src->d_w, src->d_h,
                   dst->planes[VPX_PLANE_Y], dst->stride[VPX_PLANE_Y],
                   dst->planes[VPX_PLANE_U], dst->stride[VPX_PLANE_U],
                   dst->planes[VPX_PLANE_V], dst->stride[VPX_PLANE_V],
                   dst->d_w, dst->d_h,
                   mode);
}

void usage_exit() {
  int i;

  fprintf(stderr, "Usage: %s <options> filename\n\n"
          "Options:\n", exec_name);
  arg_show_usage(stderr, all_args);
#if CONFIG_VP8_DECODER
  fprintf(stderr, "\nVP8 Postprocessing Options:\n");
  arg_show_usage(stderr, vp8_pp_args);
#endif
  fprintf(stderr,
          "\nOutput File Patterns:\n\n"
          "  The -o argument specifies the name of the file(s) to "
          "write to. If the\n  argument does not include any escape "
          "characters, the output will be\n  written to a single file. "
          "Otherwise, the filename will be calculated by\n  expanding "
          "the following escape characters:\n");
  fprintf(stderr,
          "\n\t%%w   - Frame width"
          "\n\t%%h   - Frame height"
          "\n\t%%<n> - Frame number, zero padded to <n> places (1..9)"
          "\n\n  Pattern arguments are only supported in conjunction "
          "with the --yv12 and\n  --i420 options. If the -o option is "
          "not specified, the output will be\n  directed to stdout.\n"
         );
  fprintf(stderr, "\nIncluded decoders:\n\n");

  for (i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++)
    fprintf(stderr, "    %-6s - %s\n",
            ifaces[i].name,
            vpx_codec_iface_name(ifaces[i].iface()));

  exit(EXIT_FAILURE);
}

static int raw_read_frame(FILE *infile, uint8_t **buffer,
                          size_t *bytes_read, size_t *buffer_size) {
  char raw_hdr[RAW_FRAME_HDR_SZ];
  size_t frame_size = 0;

  if (fread(raw_hdr, RAW_FRAME_HDR_SZ, 1, infile) != 1) {
    if (!feof(infile))
      warn("Failed to read RAW frame size\n");
  } else {
    const int kCorruptFrameThreshold = 256 * 1024 * 1024;
    const int kFrameTooSmallThreshold = 256 * 1024;
    frame_size = mem_get_le32(raw_hdr);

    if (frame_size > kCorruptFrameThreshold) {
      warn("Read invalid frame size (%u)\n", (unsigned int)frame_size);
      frame_size = 0;
    }

    if (frame_size < kFrameTooSmallThreshold) {
      warn("Warning: Read invalid frame size (%u) - not a raw file?\n",
           (unsigned int)frame_size);
    }

    if (frame_size > *buffer_size) {
      uint8_t *new_buf = realloc(*buffer, 2 * frame_size);
      if (new_buf) {
        *buffer = new_buf;
        *buffer_size = 2 * frame_size;
      } else {
        warn("Failed to allocate compressed data buffer\n");
        frame_size = 0;
      }
    }
  }

  if (!feof(infile)) {
    if (fread(*buffer, 1, frame_size, infile) != frame_size) {
      warn("Failed to read full frame\n");
      return 1;
    }
    *bytes_read = frame_size;
  }

  return 0;
}

static int read_frame(struct VpxDecInputContext *input, uint8_t **buf,
                      size_t *bytes_in_buffer, size_t *buffer_size) {
  switch (input->vpx_input_ctx->file_type) {
    case FILE_TYPE_WEBM:
      return webm_read_frame(input->webm_ctx,
                             buf, bytes_in_buffer, buffer_size);
    case FILE_TYPE_RAW:
      return raw_read_frame(input->vpx_input_ctx->file,
                            buf, bytes_in_buffer, buffer_size);
    case FILE_TYPE_IVF:
      return ivf_read_frame(input->vpx_input_ctx->file,
                            buf, bytes_in_buffer, buffer_size);
    default:
      return 1;
  }
}

void *out_open(const char *out_fn, int do_md5) {
  void *out = NULL;

  if (do_md5) {
    MD5Context *md5_ctx = out = malloc(sizeof(MD5Context));
    (void)out_fn;
    MD5Init(md5_ctx);
  } else {
    FILE *outfile = out = strcmp("-", out_fn) ? fopen(out_fn, "wb")
                          : set_binary_mode(stdout);

    if (!outfile) {
      fatal("Failed to output file");
    }
  }

  return out;
}

static int get_image_plane_width(int plane, const vpx_image_t *img) {
  return (plane > 0 && img->x_chroma_shift > 0) ?
             (img->d_w + 1) >> img->x_chroma_shift :
             img->d_w;
}

static int get_image_plane_height(int plane, const vpx_image_t *img) {
  return (plane > 0 &&  img->y_chroma_shift > 0) ?
             (img->d_h + 1) >> img->y_chroma_shift :
             img->d_h;
}

static void update_image_md5(const vpx_image_t *img, const int planes[3],
                             MD5Context *md5) {
  int i, y;

  for (i = 0; i < 3; ++i) {
    const int plane = planes[i];
    const unsigned char *buf = img->planes[plane];
    const int stride = img->stride[plane];
    const int w = get_image_plane_width(plane, img);
    const int h = get_image_plane_height(plane, img);

    for (y = 0; y < h; ++y) {
      MD5Update(md5, buf, w);
      buf += stride;
    }
  }
}

static void write_image_file(const vpx_image_t *img, const int planes[3],
                             FILE *file) {
  int i, y;

  for (i = 0; i < 3; ++i) {
    const int plane = planes[i];
    const unsigned char *buf = img->planes[plane];
    const int stride = img->stride[plane];
    const int w = get_image_plane_width(plane, img);
    const int h = get_image_plane_height(plane, img);

    for (y = 0; y < h; ++y) {
      fwrite(buf, 1, w, file);
      buf += stride;
    }
  }
}

void out_close(void *out, const char *out_fn, int do_md5) {
  if (do_md5) {
    uint8_t md5[16];
    int i;

    MD5Final(md5, out);
    free(out);

    for (i = 0; i < 16; i++)
      printf("%02x", md5[i]);

    printf("  %s\n", out_fn);
  } else {
    fclose(out);
  }
}

int file_is_raw(struct VpxInputContext *input) {
  uint8_t buf[32];
  int is_raw = 0;
  vpx_codec_stream_info_t si;

  si.sz = sizeof(si);

  if (fread(buf, 1, 32, input->file) == 32) {
    int i;

    if (mem_get_le32(buf) < 256 * 1024 * 1024) {
      for (i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++) {
        if (!vpx_codec_peek_stream_info(ifaces[i].iface(),
                                        buf + 4, 32 - 4, &si)) {
          is_raw = 1;
          input->fourcc = ifaces[i].fourcc;
          input->width = si.w;
          input->height = si.h;
          input->framerate.numerator = 30;
          input->framerate.denominator = 1;
          break;
        }
      }
    }
  }

  rewind(input->file);
  return is_raw;
}

void show_progress(int frame_in, int frame_out, unsigned long dx_time) {
  fprintf(stderr, "%d decoded frames/%d showed frames in %lu us (%.2f fps)\r",
          frame_in, frame_out, dx_time,
          (float)frame_out * 1000000.0 / (float)dx_time);
}

// Called by libvpx if the frame buffer size needs to increase.
//
// Parameters:
// user_priv    Data passed into libvpx.
// new_size     Minimum size needed by libvpx to decompress the next frame.
// fb           Pointer to the frame buffer to update.
//
// Returns 0 on success. Returns < 0 on failure.
int realloc_vp9_frame_buffer(void *user_priv, size_t new_size,
                             vpx_codec_frame_buffer_t *fb) {
  (void)user_priv;
  if (!fb)
    return -1;

  free(fb->data);
  fb->data = (uint8_t*)malloc(new_size);
  if (!fb->data) {
    fb->size = 0;
    return -1;
  }

  fb->size = new_size;
  return 0;
}

void generate_filename(const char *pattern, char *out, size_t q_len,
                       unsigned int d_w, unsigned int d_h,
                       unsigned int frame_in) {
  const char *p = pattern;
  char *q = out;

  do {
    char *next_pat = strchr(p, '%');

    if (p == next_pat) {
      size_t pat_len;

      /* parse the pattern */
      q[q_len - 1] = '\0';
      switch (p[1]) {
        case 'w':
          snprintf(q, q_len - 1, "%d", d_w);
          break;
        case 'h':
          snprintf(q, q_len - 1, "%d", d_h);
          break;
        case '1':
          snprintf(q, q_len - 1, "%d", frame_in);
          break;
        case '2':
          snprintf(q, q_len - 1, "%02d", frame_in);
          break;
        case '3':
          snprintf(q, q_len - 1, "%03d", frame_in);
          break;
        case '4':
          snprintf(q, q_len - 1, "%04d", frame_in);
          break;
        case '5':
          snprintf(q, q_len - 1, "%05d", frame_in);
          break;
        case '6':
          snprintf(q, q_len - 1, "%06d", frame_in);
          break;
        case '7':
          snprintf(q, q_len - 1, "%07d", frame_in);
          break;
        case '8':
          snprintf(q, q_len - 1, "%08d", frame_in);
          break;
        case '9':
          snprintf(q, q_len - 1, "%09d", frame_in);
          break;
        default:
          die("Unrecognized pattern %%%c\n", p[1]);
      }

      pat_len = strlen(q);
      if (pat_len >= q_len - 1)
        die("Output filename too long.\n");
      q += pat_len;
      p += 2;
      q_len -= pat_len;
    } else {
      size_t copy_len;

      /* copy the next segment */
      if (!next_pat)
        copy_len = strlen(p);
      else
        copy_len = next_pat - p;

      if (copy_len >= q_len - 1)
        die("Output filename too long.\n");

      memcpy(q, p, copy_len);
      q[copy_len] = '\0';
      q += copy_len;
      p += copy_len;
      q_len -= copy_len;
    }
  } while (*p);
}

int main_loop(int argc, const char **argv_) {
  vpx_codec_ctx_t       decoder;
  char                  *fn = NULL;
  int                    i;
  uint8_t               *buf = NULL;
  size_t                 bytes_in_buffer = 0, buffer_size = 0;
  FILE                  *infile;
  int                    frame_in = 0, frame_out = 0, flipuv = 0, noblit = 0;
  int                    do_md5 = 0, progress = 0;
  int                    stop_after = 0, postproc = 0, summary = 0, quiet = 1;
  int                    arg_skip = 0;
  int                    ec_enabled = 0;
  vpx_codec_iface_t       *iface = NULL;
  unsigned long          dx_time = 0;
  struct arg               arg;
  char                   **argv, **argi, **argj;
  const char             *outfile_pattern = 0;
  char                    outfile[PATH_MAX];
  int                     single_file;
  int                     use_y4m = 1;
  void                   *out = NULL;
  vpx_codec_dec_cfg_t     cfg = {0};
#if CONFIG_VP8_DECODER
  vp8_postproc_cfg_t      vp8_pp_cfg = {0};
  int                     vp8_dbg_color_ref_frame = 0;
  int                     vp8_dbg_color_mb_modes = 0;
  int                     vp8_dbg_color_b_modes = 0;
  int                     vp8_dbg_display_mv = 0;
#endif
  int                     frames_corrupted = 0;
  int                     dec_flags = 0;
  int                     do_scale = 0;
  vpx_image_t             *scaled_img = NULL;
  int                     frame_avail, got_data;
  int                     num_external_frame_buffers = 0;
  int                     fb_lru_cache = 0;
  vpx_codec_frame_buffer_t *frame_buffers = NULL;

  struct VpxDecInputContext input = {0};
  struct VpxInputContext vpx_input_ctx = {0};
  struct WebmInputContext webm_ctx = {0};
  input.vpx_input_ctx = &vpx_input_ctx;
  input.webm_ctx = &webm_ctx;

  /* Parse command line */
  exec_name = argv_[0];
  argv = argv_dup(argc - 1, argv_ + 1);

  for (argi = argj = argv; (*argj = *argi); argi += arg.argv_step) {
    memset(&arg, 0, sizeof(arg));
    arg.argv_step = 1;

    if (arg_match(&arg, &codecarg, argi)) {
      int j, k = -1;

      for (j = 0; j < sizeof(ifaces) / sizeof(ifaces[0]); j++)
        if (!strcmp(ifaces[j].name, arg.val))
          k = j;

      if (k >= 0)
        iface = ifaces[k].iface();
      else
        die("Error: Unrecognized argument (%s) to --codec\n",
            arg.val);
    } else if (arg_match(&arg, &looparg, argi)) {
      // no-op
    } else if (arg_match(&arg, &outputfile, argi))
      outfile_pattern = arg.val;
    else if (arg_match(&arg, &use_yv12, argi)) {
      use_y4m = 0;
      flipuv = 1;
    } else if (arg_match(&arg, &use_i420, argi)) {
      use_y4m = 0;
      flipuv = 0;
    } else if (arg_match(&arg, &flipuvarg, argi))
      flipuv = 1;
    else if (arg_match(&arg, &noblitarg, argi))
      noblit = 1;
    else if (arg_match(&arg, &progressarg, argi))
      progress = 1;
    else if (arg_match(&arg, &limitarg, argi))
      stop_after = arg_parse_uint(&arg);
    else if (arg_match(&arg, &skiparg, argi))
      arg_skip = arg_parse_uint(&arg);
    else if (arg_match(&arg, &postprocarg, argi))
      postproc = 1;
    else if (arg_match(&arg, &md5arg, argi))
      do_md5 = 1;
    else if (arg_match(&arg, &summaryarg, argi))
      summary = 1;
    else if (arg_match(&arg, &threadsarg, argi))
      cfg.threads = arg_parse_uint(&arg);
    else if (arg_match(&arg, &verbosearg, argi))
      quiet = 0;
    else if (arg_match(&arg, &scalearg, argi))
      do_scale = 1;
    else if (arg_match(&arg, &fb_arg, argi))
      num_external_frame_buffers = arg_parse_uint(&arg);
    else if (arg_match(&arg, &fb_lru_arg, argi))
      fb_lru_cache = arg_parse_uint(&arg);

#if CONFIG_VP8_DECODER
    else if (arg_match(&arg, &addnoise_level, argi)) {
      postproc = 1;
      vp8_pp_cfg.post_proc_flag |= VP8_ADDNOISE;
      vp8_pp_cfg.noise_level = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &demacroblock_level, argi)) {
      postproc = 1;
      vp8_pp_cfg.post_proc_flag |= VP8_DEMACROBLOCK;
      vp8_pp_cfg.deblocking_level = arg_parse_uint(&arg);
    } else if (arg_match(&arg, &deblock, argi)) {
      postproc = 1;
      vp8_pp_cfg.post_proc_flag |= VP8_DEBLOCK;
    } else if (arg_match(&arg, &mfqe, argi)) {
      postproc = 1;
      vp8_pp_cfg.post_proc_flag |= VP8_MFQE;
    } else if (arg_match(&arg, &pp_debug_info, argi)) {
      unsigned int level = arg_parse_uint(&arg);

      postproc = 1;
      vp8_pp_cfg.post_proc_flag &= ~0x7;

      if (level)
        vp8_pp_cfg.post_proc_flag |= level;
    } else if (arg_match(&arg, &pp_disp_ref_frame, argi)) {
      unsigned int flags = arg_parse_int(&arg);
      if (flags) {
        postproc = 1;
        vp8_dbg_color_ref_frame = flags;
      }
    } else if (arg_match(&arg, &pp_disp_mb_modes, argi)) {
      unsigned int flags = arg_parse_int(&arg);
      if (flags) {
        postproc = 1;
        vp8_dbg_color_mb_modes = flags;
      }
    } else if (arg_match(&arg, &pp_disp_b_modes, argi)) {
      unsigned int flags = arg_parse_int(&arg);
      if (flags) {
        postproc = 1;
        vp8_dbg_color_b_modes = flags;
      }
    } else if (arg_match(&arg, &pp_disp_mvs, argi)) {
      unsigned int flags = arg_parse_int(&arg);
      if (flags) {
        postproc = 1;
        vp8_dbg_display_mv = flags;
      }
    } else if (arg_match(&arg, &error_concealment, argi)) {
      ec_enabled = 1;
    }

#endif
    else
      argj++;
  }

  /* Check for unrecognized options */
  for (argi = argv; *argi; argi++)
    if (argi[0][0] == '-' && strlen(argi[0]) > 1)
      die("Error: Unrecognized option %s\n", *argi);

  /* Handle non-option arguments */
  fn = argv[0];

  if (!fn)
    usage_exit();

  /* Open file */
  infile = strcmp(fn, "-") ? fopen(fn, "rb") : set_binary_mode(stdin);

  if (!infile) {
    fprintf(stderr, "Failed to open file '%s'",
            strcmp(fn, "-") ? fn : "stdin");
    return EXIT_FAILURE;
  }
#if CONFIG_OS_SUPPORT
  /* Make sure we don't dump to the terminal, unless forced to with -o - */
  if (!outfile_pattern && isatty(fileno(stdout)) && !do_md5 && !noblit) {
    fprintf(stderr,
            "Not dumping raw video to your terminal. Use '-o -' to "
            "override.\n");
    return EXIT_FAILURE;
  }
#endif
  input.vpx_input_ctx->file = infile;
  if (file_is_ivf(input.vpx_input_ctx))
    input.vpx_input_ctx->file_type = FILE_TYPE_IVF;
  else if (file_is_webm(input.webm_ctx, input.vpx_input_ctx))
    input.vpx_input_ctx->file_type = FILE_TYPE_WEBM;
  else if (file_is_raw(input.vpx_input_ctx))
    input.vpx_input_ctx->file_type = FILE_TYPE_RAW;
  else {
    fprintf(stderr, "Unrecognized input file type.\n");
    return EXIT_FAILURE;
  }

  /* If the output file is not set or doesn't have a sequence number in
   * it, then we only open it once.
   */
  outfile_pattern = outfile_pattern ? outfile_pattern : "-";
  single_file = 1;
  {
    const char *p = outfile_pattern;
    do {
      p = strchr(p, '%');
      if (p && p[1] >= '1' && p[1] <= '9') {
        /* pattern contains sequence number, so it's not unique. */
        single_file = 0;
        break;
      }
      if (p)
        p++;
    } while (p);
  }

  if (single_file && !noblit) {
    generate_filename(outfile_pattern, outfile, sizeof(outfile) - 1,
                      vpx_input_ctx.width, vpx_input_ctx.height, 0);
    out = out_open(outfile, do_md5);
  }

  if (use_y4m && !noblit) {
    if (!single_file) {
      fprintf(stderr, "YUV4MPEG2 not supported with output patterns,"
              " try --i420 or --yv12.\n");
      return EXIT_FAILURE;
    }

    if (vpx_input_ctx.file_type == FILE_TYPE_WEBM) {
      if (webm_guess_framerate(input.webm_ctx, input.vpx_input_ctx)) {
        fprintf(stderr, "Failed to guess framerate -- error parsing "
                "webm file?\n");
        return EXIT_FAILURE;
      }
    }
  }

  /* Try to determine the codec from the fourcc. */
  for (i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++)
    if (vpx_input_ctx.fourcc == ifaces[i].fourcc) {
      vpx_codec_iface_t *vpx_iface = ifaces[i].iface();

      if (iface && iface != vpx_iface)
        warn("Header indicates codec: %s\n", ifaces[i].name);
      else
        iface = vpx_iface;

      break;
    }

  dec_flags = (postproc ? VPX_CODEC_USE_POSTPROC : 0) |
              (ec_enabled ? VPX_CODEC_USE_ERROR_CONCEALMENT : 0);
  if (vpx_codec_dec_init(&decoder, iface ? iface :  ifaces[0].iface(), &cfg,
                         dec_flags)) {
    fprintf(stderr, "Failed to initialize decoder: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (!quiet)
    fprintf(stderr, "%s\n", decoder.name);

#if CONFIG_VP8_DECODER

  if (vp8_pp_cfg.post_proc_flag
      && vpx_codec_control(&decoder, VP8_SET_POSTPROC, &vp8_pp_cfg)) {
    fprintf(stderr, "Failed to configure postproc: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (vp8_dbg_color_ref_frame
      && vpx_codec_control(&decoder, VP8_SET_DBG_COLOR_REF_FRAME, vp8_dbg_color_ref_frame)) {
    fprintf(stderr, "Failed to configure reference block visualizer: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (vp8_dbg_color_mb_modes
      && vpx_codec_control(&decoder, VP8_SET_DBG_COLOR_MB_MODES, vp8_dbg_color_mb_modes)) {
    fprintf(stderr, "Failed to configure macro block visualizer: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (vp8_dbg_color_b_modes
      && vpx_codec_control(&decoder, VP8_SET_DBG_COLOR_B_MODES, vp8_dbg_color_b_modes)) {
    fprintf(stderr, "Failed to configure block visualizer: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (vp8_dbg_display_mv
      && vpx_codec_control(&decoder, VP8_SET_DBG_DISPLAY_MV, vp8_dbg_display_mv)) {
    fprintf(stderr, "Failed to configure motion vector visualizer: %s\n", vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }
#endif


  if (arg_skip)
    fprintf(stderr, "Skipping first %d frames.\n", arg_skip);
  while (arg_skip) {
    if (read_frame(&input, &buf, &bytes_in_buffer, &buffer_size))
      break;
    arg_skip--;
  }

  if (num_external_frame_buffers > 0) {
    // Allocate the frame buffer list, setting all of the values to 0.
    // Including the size of frame buffers. Libvpx will request the
    // application to realloc the frame buffer data if the size is too small.
    frame_buffers = (vpx_codec_frame_buffer_t*)calloc(
        num_external_frame_buffers, sizeof(*frame_buffers));
    if (vpx_codec_set_frame_buffers(&decoder, frame_buffers,
                                    num_external_frame_buffers,
                                    realloc_vp9_frame_buffer,
                                    NULL)) {
      fprintf(stderr, "Failed to configure external frame buffers: %s\n",
              vpx_codec_error(&decoder));
      return EXIT_FAILURE;
    }
  }

  if (fb_lru_cache > 0 &&
      vpx_codec_control(&decoder, VP9D_SET_FRAME_BUFFER_LRU_CACHE,
                        fb_lru_cache)) {
    fprintf(stderr, "Failed to set frame buffer lru cache: %s\n",
            vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  frame_avail = 1;
  got_data = 0;

  /* Decode file */
  while (frame_avail || got_data) {
    vpx_codec_iter_t  iter = NULL;
    vpx_image_t    *img;
    struct vpx_usec_timer timer;
    int                   corrupted;

    frame_avail = 0;
    if (!stop_after || frame_in < stop_after) {
      if (!read_frame(&input, &buf, &bytes_in_buffer, &buffer_size)) {
        frame_avail = 1;
        frame_in++;

        vpx_usec_timer_start(&timer);

        if (vpx_codec_decode(&decoder, buf, bytes_in_buffer, NULL, 0)) {
          const char *detail = vpx_codec_error_detail(&decoder);
          warn("Failed to decode frame %d: %s",
               frame_in, vpx_codec_error(&decoder));

          if (detail)
            warn("Additional information: %s", detail);
          goto fail;
        }

        vpx_usec_timer_mark(&timer);
        dx_time += (unsigned int)vpx_usec_timer_elapsed(&timer);
      }
    }

    vpx_usec_timer_start(&timer);

    got_data = 0;
    if ((img = vpx_codec_get_frame(&decoder, &iter))) {
      ++frame_out;
      got_data = 1;
    }

    vpx_usec_timer_mark(&timer);
    dx_time += (unsigned int)vpx_usec_timer_elapsed(&timer);

    if (vpx_codec_control(&decoder, VP8D_GET_FRAME_CORRUPTED, &corrupted)) {
      warn("Failed VP8_GET_FRAME_CORRUPTED: %s", vpx_codec_error(&decoder));
      goto fail;
    }
    frames_corrupted += corrupted;

    if (progress)
      show_progress(frame_in, frame_out, dx_time);

    if (!noblit) {
      if (frame_out == 1 && img && use_y4m) {
        y4m_write_file_header(out, vpx_input_ctx.width, vpx_input_ctx.height,
                              &vpx_input_ctx.framerate, img->fmt);
      }

      if (img && do_scale) {
        if (frame_out == 1) {
          // If the output frames are to be scaled to a fixed display size then
          // use the width and height specified in the container. If either of
          // these is set to 0, use the display size set in the first frame
          // header.
          int display_width = vpx_input_ctx.width;
          int display_height = vpx_input_ctx.height;
          if (!display_width || !display_height) {
            int display_size[2];
            if (vpx_codec_control(&decoder, VP9D_GET_DISPLAY_SIZE,
                                  display_size)) {
              // As last resort use size of first frame as display size.
              display_width = img->d_w;
              display_height = img->d_h;
            } else {
              display_width = display_size[0];
              display_height = display_size[1];
            }
          }
          scaled_img = vpx_img_alloc(NULL, VPX_IMG_FMT_I420, display_width,
                                     display_height, 16);
        }

        if (img->d_w != scaled_img->d_w || img->d_h != scaled_img->d_h) {
          vpx_image_scale(img, scaled_img, kFilterBox);
          img = scaled_img;
        }
      }

      if (img) {
        const int PLANES_YUV[] = {VPX_PLANE_Y, VPX_PLANE_U, VPX_PLANE_V};
        const int PLANES_YVU[] = {VPX_PLANE_Y, VPX_PLANE_V, VPX_PLANE_U};

        const int *planes = flipuv ? PLANES_YVU : PLANES_YUV;
        char out_fn[PATH_MAX];

        if (!single_file) {
          generate_filename(outfile_pattern, out_fn, PATH_MAX,
                            img->d_w, img->d_h, frame_in);
          out = out_open(out_fn, do_md5);
        } else {
          if (use_y4m)
            y4m_write_frame_header(out);
        }

        if (do_md5)
          update_image_md5(img, planes, out);
        else
          write_image_file(img, planes, out);

        if (!single_file)
          out_close(out, out_fn, do_md5);
      }
    }

    if (stop_after && frame_in >= stop_after)
      break;
  }

  if (summary || progress) {
    show_progress(frame_in, frame_out, dx_time);
    fprintf(stderr, "\n");
  }

  if (frames_corrupted)
    fprintf(stderr, "WARNING: %d frames corrupted.\n", frames_corrupted);

fail:

  if (vpx_codec_destroy(&decoder)) {
    fprintf(stderr, "Failed to destroy decoder: %s\n",
            vpx_codec_error(&decoder));
    return EXIT_FAILURE;
  }

  if (single_file && !noblit)
    out_close(out, outfile, do_md5);

  if (input.vpx_input_ctx->file_type == FILE_TYPE_WEBM)
    webm_free(input.webm_ctx);
  else
    free(buf);

  if (scaled_img) vpx_img_free(scaled_img);
  for (i = 0; i < num_external_frame_buffers; ++i) {
    free(frame_buffers[i].data);
  }
  free(frame_buffers);

  fclose(infile);
  free(argv);

  return frames_corrupted ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, const char **argv_) {
  unsigned int loops = 1, i;
  char **argv, **argi, **argj;
  struct arg arg;
  int error = 0;

  argv = argv_dup(argc - 1, argv_ + 1);
  for (argi = argj = argv; (*argj = *argi); argi += arg.argv_step) {
    memset(&arg, 0, sizeof(arg));
    arg.argv_step = 1;

    if (arg_match(&arg, &looparg, argi)) {
      loops = arg_parse_uint(&arg);
      break;
    }
  }
  free(argv);
  for (i = 0; !error && i < loops; i++)
    error = main_loop(argc, argv_);
  return error;
}
