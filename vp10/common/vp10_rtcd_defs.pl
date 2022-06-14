sub vp10_common_forward_decls() {
print <<EOF
/*
 * VP10
 */

#include "vpx/vpx_integer.h"
#include "vp10/common/common.h"
#include "vp10/common/enums.h"
#include "vp10/common/quant_common.h"
#include "vp10/common/filter.h"
#include "vp10/common/vp10_txfm.h"

struct macroblockd;

/* Encoder forward decls */
struct macroblock;
struct vpx_variance_vtable;
struct search_site_config;
struct mv;
union int_mv;
struct yv12_buffer_config;
EOF
}
forward_decls qw/vp10_common_forward_decls/;

# functions that are 64 bit only.
$mmx_x86_64 = $sse2_x86_64 = $ssse3_x86_64 = $avx_x86_64 = $avx2_x86_64 = '';
if ($opts{arch} eq "x86_64") {
  $mmx_x86_64 = 'mmx';
  $sse2_x86_64 = 'sse2';
  $ssse3_x86_64 = 'ssse3';
  $avx_x86_64 = 'avx';
  $avx2_x86_64 = 'avx2';
}

#
# 10/12-tap convolution filters
#
add_proto qw/void vp10_convolve_horiz/, "const uint8_t *src, int src_stride, uint8_t *dst, int dst_stride, int w, int h, const InterpFilterParams fp, const int subpel_x_q4, int x_step_q4, int avg";
specialize qw/vp10_convolve_horiz ssse3/;

add_proto qw/void vp10_convolve_vert/, "const uint8_t *src, int src_stride, uint8_t *dst, int dst_stride, int w, int h, const InterpFilterParams fp, const int subpel_x_q4, int x_step_q4, int avg";
specialize qw/vp10_convolve_vert ssse3/;

if (vpx_config("CONFIG_VPX_HIGHBITDEPTH") eq "yes") {
  add_proto qw/void vp10_highbd_convolve_horiz/, "const uint16_t *src, int src_stride, uint16_t *dst, int dst_stride, int w, int h, const InterpFilterParams fp, const int subpel_x_q4, int x_step_q4, int avg, int bd";
  specialize qw/vp10_highbd_convolve_horiz sse4_1/;
  add_proto qw/void vp10_highbd_convolve_vert/, "const uint16_t *src, int src_stride, uint16_t *dst, int dst_stride, int w, int h, const InterpFilterParams fp, const int subpel_x_q4, int x_step_q4, int avg, int bd";
  specialize qw/vp10_highbd_convolve_vert sse4_1/;
}

#
# dct
#
if (vpx_config("CONFIG_VPX_HIGHBITDEPTH") eq "yes") {
  # Note as optimized versions of these functions are added we need to add a check to ensure
  # that when CONFIG_EMULATE_HARDWARE is on, it defaults to the C versions only.
  if (vpx_config("CONFIG_EMULATE_HARDWARE") eq "yes") {
    add_proto qw/void vp10_iht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht4x4_16_add/;

    add_proto qw/void vp10_iht8x4_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht8x4_32_add/;

    add_proto qw/void vp10_iht4x8_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht4x8_32_add/;

    add_proto qw/void vp10_iht8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht8x8_64_add/;

    add_proto qw/void vp10_iht16x16_256_add/, "const tran_low_t *input, uint8_t *output, int pitch, int tx_type";
    specialize qw/vp10_iht16x16_256_add/;

    add_proto qw/void vp10_fdct4x4/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct4x4/;

    add_proto qw/void vp10_fdct4x4_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct4x4_1/;

    add_proto qw/void vp10_fdct8x8/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct8x8/;

    add_proto qw/void vp10_fdct8x8_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct8x8_1/;

    add_proto qw/void vp10_fdct16x16/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct16x16/;

    add_proto qw/void vp10_fdct16x16_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct16x16_1/;

    add_proto qw/void vp10_fdct32x32/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32/;

    add_proto qw/void vp10_fdct32x32_rd/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32_rd/;

    add_proto qw/void vp10_fdct32x32_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32_1/;

    add_proto qw/void vp10_highbd_fdct4x4/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct4x4/;

    add_proto qw/void vp10_highbd_fdct8x8/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct8x8/;

    add_proto qw/void vp10_highbd_fdct8x8_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct8x8_1/;

    add_proto qw/void vp10_highbd_fdct16x16/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct16x16/;

    add_proto qw/void vp10_highbd_fdct16x16_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct16x16_1/;

    add_proto qw/void vp10_highbd_fdct32x32/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct32x32/;

    add_proto qw/void vp10_highbd_fdct32x32_rd/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct32x32_rd/;

    add_proto qw/void vp10_highbd_fdct32x32_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct32x32_1/;
  } else {
    add_proto qw/void vp10_iht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht4x4_16_add sse2/;

    add_proto qw/void vp10_iht8x4_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht8x4_32_add/;

    add_proto qw/void vp10_iht4x8_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht4x8_32_add/;

    add_proto qw/void vp10_iht8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht8x8_64_add sse2/;

    add_proto qw/void vp10_iht16x16_256_add/, "const tran_low_t *input, uint8_t *output, int pitch, int tx_type";
    specialize qw/vp10_iht16x16_256_add sse2/;

    add_proto qw/void vp10_fdct4x4/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct4x4 sse2/;

    add_proto qw/void vp10_fdct4x4_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct4x4_1 sse2/;

    add_proto qw/void vp10_fdct8x8/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct8x8 sse2/;

    add_proto qw/void vp10_fdct8x8_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct8x8_1 sse2/;

    add_proto qw/void vp10_fdct16x16/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct16x16 sse2/;

    add_proto qw/void vp10_fdct16x16_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct16x16_1 sse2/;

    add_proto qw/void vp10_fdct32x32/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32 sse2/;

    add_proto qw/void vp10_fdct32x32_rd/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32_rd sse2/;

    add_proto qw/void vp10_fdct32x32_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32_1 sse2/;

    add_proto qw/void vp10_highbd_fdct4x4/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct4x4 sse2/;

    add_proto qw/void vp10_highbd_fdct8x8/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct8x8 sse2/;

    add_proto qw/void vp10_highbd_fdct8x8_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct8x8_1/;

    add_proto qw/void vp10_highbd_fdct16x16/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct16x16 sse2/;

    add_proto qw/void vp10_highbd_fdct16x16_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct16x16_1/;

    add_proto qw/void vp10_highbd_fdct32x32/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct32x32 sse2/;

    add_proto qw/void vp10_highbd_fdct32x32_rd/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct32x32_rd sse2/;

    add_proto qw/void vp10_highbd_fdct32x32_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_highbd_fdct32x32_1/;
  }
} else {
  # Force C versions if CONFIG_EMULATE_HARDWARE is 1
  if (vpx_config("CONFIG_EMULATE_HARDWARE") eq "yes") {
    add_proto qw/void vp10_iht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht4x4_16_add/;

    add_proto qw/void vp10_iht8x4_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht8x4_32_add/;

    add_proto qw/void vp10_iht4x8_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht4x8_32_add/;

    add_proto qw/void vp10_iht8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht8x8_64_add/;

    add_proto qw/void vp10_iht16x16_256_add/, "const tran_low_t *input, uint8_t *output, int pitch, int tx_type";
    specialize qw/vp10_iht16x16_256_add/;

    add_proto qw/void vp10_fdct4x4/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct4x4/;

    add_proto qw/void vp10_fdct4x4_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct4x4_1/;

    add_proto qw/void vp10_fdct8x8/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct8x8/;

    add_proto qw/void vp10_fdct8x8_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct8x8_1/;

    add_proto qw/void vp10_fdct16x16/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct16x16/;

    add_proto qw/void vp10_fdct16x16_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct16x16_1/;

    add_proto qw/void vp10_fdct32x32/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32/;

    add_proto qw/void vp10_fdct32x32_rd/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32_rd/;

    add_proto qw/void vp10_fdct32x32_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32_1/;
  } else {
    add_proto qw/void vp10_iht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht4x4_16_add sse2 neon dspr2/;

    add_proto qw/void vp10_iht8x4_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht8x4_32_add/;

    add_proto qw/void vp10_iht4x8_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht4x8_32_add/;

    add_proto qw/void vp10_iht8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type";
    specialize qw/vp10_iht8x8_64_add sse2 neon dspr2/;

    add_proto qw/void vp10_iht16x16_256_add/, "const tran_low_t *input, uint8_t *output, int pitch, int tx_type";
    specialize qw/vp10_iht16x16_256_add sse2 dspr2/;

    if (vpx_config("CONFIG_EXT_TX") ne "yes") {
      specialize qw/vp10_iht4x4_16_add msa/;
      specialize qw/vp10_iht8x8_64_add msa/;
      specialize qw/vp10_iht16x16_256_add msa/;
    }

    add_proto qw/void vp10_fdct4x4/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct4x4 sse2/;

    add_proto qw/void vp10_fdct4x4_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct4x4_1 sse2/;

    add_proto qw/void vp10_fdct8x8/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct8x8 sse2/;

    add_proto qw/void vp10_fdct8x8_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct8x8_1 sse2/;

    add_proto qw/void vp10_fdct16x16/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct16x16 sse2/;

    add_proto qw/void vp10_fdct16x16_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct16x16_1 sse2/;

    add_proto qw/void vp10_fdct32x32/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32 sse2/;

    add_proto qw/void vp10_fdct32x32_rd/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32_rd sse2/;

    add_proto qw/void vp10_fdct32x32_1/, "const int16_t *input, tran_low_t *output, int stride";
    specialize qw/vp10_fdct32x32_1 sse2/;
  }
}

if (vpx_config("CONFIG_NEW_QUANT") eq "yes") {
  add_proto qw/void quantize_nuq/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, const int16_t *dequant_ptr, const cuml_bins_type_nuq *cuml_bins_ptr, const dequant_val_type_nuq *dequant_val, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, uint16_t *eob_ptr, const int16_t *scan, const uint8_t *band";
  specialize qw/quantize_nuq/;

  add_proto qw/void quantize_fp_nuq/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *quant_ptr, const int16_t *dequant_ptr, const cuml_bins_type_nuq *cuml_bins_ptr, const dequant_val_type_nuq *dequant_val, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, uint16_t *eob_ptr, const int16_t *scan, const uint8_t *band";
  specialize qw/quantize_fp_nuq/;

  add_proto qw/void quantize_32x32_nuq/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, const int16_t *dequant_ptr, const cuml_bins_type_nuq *cuml_bins_ptr, const dequant_val_type_nuq *dequant_val, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, uint16_t *eob_ptr, const int16_t *scan, const uint8_t *band";
  specialize qw/quantize_32x32_nuq/;

  add_proto qw/void quantize_32x32_fp_nuq/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *quant_ptr, const int16_t *dequant_ptr, const cuml_bins_type_nuq *cuml_bins_ptr, const dequant_val_type_nuq *dequant_val, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, uint16_t *eob_ptr, const int16_t *scan, const uint8_t *band";
  specialize qw/quantize_32x32_fp_nuq/;
}

# High bitdepth functions
if (vpx_config("CONFIG_VPX_HIGHBITDEPTH") eq "yes") {
  #
  # Sub Pixel Filters
  #
  add_proto qw/void vp10_highbd_convolve_copy/, "const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const int16_t *filter_x, int x_step_q4, const int16_t *filter_y, int y_step_q4, int w, int h, int bps";
  specialize qw/vp10_highbd_convolve_copy/;

  add_proto qw/void vp10_highbd_convolve_avg/, "const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const int16_t *filter_x, int x_step_q4, const int16_t *filter_y, int y_step_q4, int w, int h, int bps";
  specialize qw/vp10_highbd_convolve_avg/;

  add_proto qw/void vp10_highbd_convolve8/, "const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const int16_t *filter_x, int x_step_q4, const int16_t *filter_y, int y_step_q4, int w, int h, int bps";
  specialize qw/vp10_highbd_convolve8/, "$sse2_x86_64";

  add_proto qw/void vp10_highbd_convolve8_horiz/, "const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const int16_t *filter_x, int x_step_q4, const int16_t *filter_y, int y_step_q4, int w, int h, int bps";
  specialize qw/vp10_highbd_convolve8_horiz/, "$sse2_x86_64";

  add_proto qw/void vp10_highbd_convolve8_vert/, "const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const int16_t *filter_x, int x_step_q4, const int16_t *filter_y, int y_step_q4, int w, int h, int bps";
  specialize qw/vp10_highbd_convolve8_vert/, "$sse2_x86_64";

  add_proto qw/void vp10_highbd_convolve8_avg/, "const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const int16_t *filter_x, int x_step_q4, const int16_t *filter_y, int y_step_q4, int w, int h, int bps";
  specialize qw/vp10_highbd_convolve8_avg/, "$sse2_x86_64";

  add_proto qw/void vp10_highbd_convolve8_avg_horiz/, "const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const int16_t *filter_x, int x_step_q4, const int16_t *filter_y, int y_step_q4, int w, int h, int bps";
  specialize qw/vp10_highbd_convolve8_avg_horiz/, "$sse2_x86_64";

  add_proto qw/void vp10_highbd_convolve8_avg_vert/, "const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const int16_t *filter_x, int x_step_q4, const int16_t *filter_y, int y_step_q4, int w, int h, int bps";
  specialize qw/vp10_highbd_convolve8_avg_vert/, "$sse2_x86_64";

  #
  # dct
  #
  # Note as optimized versions of these functions are added we need to add a check to ensure
  # that when CONFIG_EMULATE_HARDWARE is on, it defaults to the C versions only.
  add_proto qw/void vp10_highbd_iht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type, int bd";
  specialize qw/vp10_highbd_iht4x4_16_add/;

  add_proto qw/void vp10_highbd_iht8x4_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type, int bd";
  specialize qw/vp10_highbd_iht8x4_32_add/;

  add_proto qw/void vp10_highbd_iht4x8_32_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type, int bd";
  specialize qw/vp10_highbd_iht4x8_32_add/;

  add_proto qw/void vp10_highbd_iht8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int tx_type, int bd";
  specialize qw/vp10_highbd_iht8x8_64_add/;

  add_proto qw/void vp10_highbd_iht16x16_256_add/, "const tran_low_t *input, uint8_t *output, int pitch, int tx_type, int bd";
  specialize qw/vp10_highbd_iht16x16_256_add/;
}

#
# Encoder functions below this point.
#
if (vpx_config("CONFIG_VP10_ENCODER") eq "yes") {

# ENCODEMB INVOKE

if (vpx_config("CONFIG_VPX_HIGHBITDEPTH") eq "yes") {
# the transform coefficients are held in 32-bit
# values, so the assembler code for  vp10_block_error can no longer be used.
  add_proto qw/int64_t vp10_block_error/, "const tran_low_t *coeff, const tran_low_t *dqcoeff, intptr_t block_size, int64_t *ssz";
  specialize qw/vp10_block_error/;

  add_proto qw/void vp10_quantize_fp/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan";
  specialize qw/vp10_quantize_fp/;

  add_proto qw/void vp10_quantize_fp_32x32/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan";
  specialize qw/vp10_quantize_fp_32x32/;

  add_proto qw/void vp10_fdct8x8_quant/, "const int16_t *input, int stride, tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan";
  specialize qw/vp10_fdct8x8_quant/;
} else {
  add_proto qw/int64_t vp10_block_error/, "const tran_low_t *coeff, const tran_low_t *dqcoeff, intptr_t block_size, int64_t *ssz";
  specialize qw/vp10_block_error sse2 avx2 msa/;

  add_proto qw/int64_t vp10_block_error_fp/, "const int16_t *coeff, const int16_t *dqcoeff, int block_size";
  specialize qw/vp10_block_error_fp neon sse2/;

  add_proto qw/void vp10_quantize_fp/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan";
  specialize qw/vp10_quantize_fp neon sse2/, "$ssse3_x86_64";

  add_proto qw/void vp10_quantize_fp_32x32/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan";
  specialize qw/vp10_quantize_fp_32x32/, "$ssse3_x86_64";

  add_proto qw/void vp10_fdct8x8_quant/, "const int16_t *input, int stride, tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan";
  specialize qw/vp10_fdct8x8_quant sse2 ssse3 neon/;
}

# fdct functions

if (vpx_config("CONFIG_VPX_HIGHBITDEPTH") eq "yes") {
  add_proto qw/void vp10_fht4x4/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht4x4 sse2/;

  add_proto qw/void vp10_fht8x4/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht8x4/;

  add_proto qw/void vp10_fht4x8/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht4x8/;

  add_proto qw/void vp10_fht8x8/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht8x8 sse2/;

  add_proto qw/void vp10_fht16x16/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht16x16 sse2/;

  add_proto qw/void vp10_fht32x32/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht32x32/;

  add_proto qw/void vp10_fwht4x4/, "const int16_t *input, tran_low_t *output, int stride";
  specialize qw/vp10_fwht4x4/;
} else {
  add_proto qw/void vp10_fht4x4/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht4x4 sse2/;

  add_proto qw/void vp10_fht8x4/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht8x4/;

  add_proto qw/void vp10_fht4x8/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht4x8/;

  add_proto qw/void vp10_fht8x8/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht8x8 sse2/;

  add_proto qw/void vp10_fht16x16/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht16x16 sse2/;

  if (vpx_config("CONFIG_EXT_TX") ne "yes") {
    specialize qw/vp10_fht4x4 msa/;
    specialize qw/vp10_fht8x8 msa/;
    specialize qw/vp10_fht16x16 msa/;
  }

  add_proto qw/void vp10_fht32x32/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_fht32x32/;

  add_proto qw/void vp10_fwht4x4/, "const int16_t *input, tran_low_t *output, int stride";
  specialize qw/vp10_fwht4x4/;
}

add_proto qw/void vp10_fwd_idtx/, "const int16_t *src_diff, tran_low_t *coeff, int stride, int bs, int tx_type";
  specialize qw/vp10_fwd_idtx/;

# Inverse transform
if (vpx_config("CONFIG_VPX_HIGHBITDEPTH") eq "yes") {
  # Note as optimized versions of these functions are added we need to add a check to ensure
  # that when CONFIG_EMULATE_HARDWARE is on, it defaults to the C versions only.
  add_proto qw/void vp10_idct4x4_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct4x4_1_add/;

  add_proto qw/void vp10_idct4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct4x4_16_add/;

  add_proto qw/void vp10_idct8x8_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct8x8_1_add/;

  add_proto qw/void vp10_idct8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct8x8_64_add/;

  add_proto qw/void vp10_idct8x8_12_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct8x8_12_add/;

  add_proto qw/void vp10_idct16x16_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct16x16_1_add/;

  add_proto qw/void vp10_idct16x16_256_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct16x16_256_add/;

  add_proto qw/void vp10_idct16x16_10_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct16x16_10_add/;

  add_proto qw/void vp10_idct32x32_1024_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct32x32_1024_add/;

  add_proto qw/void vp10_idct32x32_34_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct32x32_34_add/;

  add_proto qw/void vp10_idct32x32_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_idct32x32_1_add/;

  add_proto qw/void vp10_iwht4x4_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_iwht4x4_1_add/;

  add_proto qw/void vp10_iwht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
  specialize qw/vp10_iwht4x4_16_add/;

  add_proto qw/void vp10_highbd_idct4x4_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
  specialize qw/vp10_highbd_idct4x4_1_add/;

  add_proto qw/void vp10_highbd_idct8x8_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
  specialize qw/vp10_highbd_idct8x8_1_add/;

  add_proto qw/void vp10_highbd_idct16x16_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
  specialize qw/vp10_highbd_idct16x16_1_add/;

  add_proto qw/void vp10_highbd_idct32x32_1024_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
  specialize qw/vp10_highbd_idct32x32_1024_add/;

  add_proto qw/void vp10_highbd_idct32x32_34_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
  specialize qw/vp10_highbd_idct32x32_34_add/;

  add_proto qw/void vp10_highbd_idct32x32_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
  specialize qw/vp10_highbd_idct32x32_1_add/;

  add_proto qw/void vp10_highbd_iwht4x4_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
  specialize qw/vp10_highbd_iwht4x4_1_add/;

  add_proto qw/void vp10_highbd_iwht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
  specialize qw/vp10_highbd_iwht4x4_16_add/;

  # Force C versions if CONFIG_EMULATE_HARDWARE is 1
  if (vpx_config("CONFIG_EMULATE_HARDWARE") eq "yes") {
    add_proto qw/void vp10_highbd_idct4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct4x4_16_add/;

    add_proto qw/void vp10_highbd_idct8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct8x8_64_add/;

    add_proto qw/void vp10_highbd_idct8x8_10_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct8x8_10_add/;

    add_proto qw/void vp10_highbd_idct16x16_256_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct16x16_256_add/;

    add_proto qw/void vp10_highbd_idct16x16_10_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct16x16_10_add/;
  } else {
    add_proto qw/void vp10_highbd_idct4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct4x4_16_add sse2/;

    add_proto qw/void vp10_highbd_idct8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct8x8_64_add sse2/;

    add_proto qw/void vp10_highbd_idct8x8_10_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct8x8_10_add sse2/;

    add_proto qw/void vp10_highbd_idct16x16_256_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct16x16_256_add sse2/;

    add_proto qw/void vp10_highbd_idct16x16_10_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride, int bd";
    specialize qw/vp10_highbd_idct16x16_10_add sse2/;
  }  # CONFIG_EMULATE_HARDWARE
} else {
  # Force C versions if CONFIG_EMULATE_HARDWARE is 1
  if (vpx_config("CONFIG_EMULATE_HARDWARE") eq "yes") {
    add_proto qw/void vp10_idct4x4_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct4x4_1_add/;

    add_proto qw/void vp10_idct4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct4x4_16_add/;

    add_proto qw/void vp10_idct8x8_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct8x8_1_add/;

    add_proto qw/void vp10_idct8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct8x8_64_add/;

    add_proto qw/void vp10_idct8x8_12_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct8x8_12_add/;

    add_proto qw/void vp10_idct16x16_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct16x16_1_add/;

    add_proto qw/void vp10_idct16x16_256_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct16x16_256_add/;

    add_proto qw/void vp10_idct16x16_10_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct16x16_10_add/;

    add_proto qw/void vp10_idct32x32_1024_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct32x32_1024_add/;

    add_proto qw/void vp10_idct32x32_34_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct32x32_34_add/;

    add_proto qw/void vp10_idct32x32_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct32x32_1_add/;

    add_proto qw/void vp10_iwht4x4_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_iwht4x4_1_add/;

    add_proto qw/void vp10_iwht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_iwht4x4_16_add/;
  } else {
    add_proto qw/void vp10_idct4x4_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct4x4_1_add sse2/;

    add_proto qw/void vp10_idct4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct4x4_16_add sse2/;

    add_proto qw/void vp10_idct8x8_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct8x8_1_add sse2/;

    add_proto qw/void vp10_idct8x8_64_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct8x8_64_add sse2/;

    add_proto qw/void vp10_idct8x8_12_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct8x8_12_add sse2/;

    add_proto qw/void vp10_idct16x16_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct16x16_1_add sse2/;

    add_proto qw/void vp10_idct16x16_256_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct16x16_256_add sse2/;

    add_proto qw/void vp10_idct16x16_10_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct16x16_10_add sse2/;

    add_proto qw/void vp10_idct32x32_1024_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct32x32_1024_add sse2/;

    add_proto qw/void vp10_idct32x32_34_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct32x32_34_add sse2/;

    add_proto qw/void vp10_idct32x32_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_idct32x32_1_add sse2/;

    add_proto qw/void vp10_iwht4x4_1_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_iwht4x4_1_add/;

    add_proto qw/void vp10_iwht4x4_16_add/, "const tran_low_t *input, uint8_t *dest, int dest_stride";
    specialize qw/vp10_iwht4x4_16_add/;
  }  # CONFIG_EMULATE_HARDWARE
}  # CONFIG_VPX_HIGHBITDEPTH

if (vpx_config("CONFIG_VPX_HIGHBITDEPTH") eq "yes") {
  #fwd txfm
  add_proto qw/void vp10_fwd_txfm2d_4x4/, "const int16_t *input, int32_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_fwd_txfm2d_4x4 sse4_1/;
  add_proto qw/void vp10_fwd_txfm2d_8x8/, "const int16_t *input, int32_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_fwd_txfm2d_8x8 sse4_1/;
  add_proto qw/void vp10_fwd_txfm2d_16x16/, "const int16_t *input, int32_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_fwd_txfm2d_16x16 sse4_1/;
  add_proto qw/void vp10_fwd_txfm2d_32x32/, "const int16_t *input, int32_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_fwd_txfm2d_32x32 sse4_1/;
  add_proto qw/void vp10_fwd_txfm2d_64x64/, "const int16_t *input, int32_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_fwd_txfm2d_64x64 sse4_1/;

  #inv txfm
  add_proto qw/void vp10_inv_txfm2d_add_4x4/, "const int32_t *input, uint16_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_inv_txfm2d_add_4x4 sse4_1/;
  add_proto qw/void vp10_inv_txfm2d_add_8x8/, "const int32_t *input, uint16_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_inv_txfm2d_add_8x8 sse4_1/;
  add_proto qw/void vp10_inv_txfm2d_add_16x16/, "const int32_t *input, uint16_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_inv_txfm2d_add_16x16 sse4_1/;
  add_proto qw/void vp10_inv_txfm2d_add_32x32/, "const int32_t *input, uint16_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_inv_txfm2d_add_32x32/;
  add_proto qw/void vp10_inv_txfm2d_add_64x64/, "const int32_t *input, uint16_t *output, int stride, int tx_type, int bd";
  specialize qw/vp10_inv_txfm2d_add_64x64/;
}

#
# Motion search
#
add_proto qw/int vp10_full_search_sad/, "const struct macroblock *x, const struct mv *ref_mv, int sad_per_bit, int distance, const struct vpx_variance_vtable *fn_ptr, const struct mv *center_mv, struct mv *best_mv";
specialize qw/vp10_full_search_sad sse3 sse4_1/;
$vp10_full_search_sad_sse3=vp10_full_search_sadx3;
$vp10_full_search_sad_sse4_1=vp10_full_search_sadx8;

add_proto qw/int vp10_diamond_search_sad/, "struct macroblock *x, const struct search_site_config *cfg,  struct mv *ref_mv, struct mv *best_mv, int search_param, int sad_per_bit, int *num00, const struct vpx_variance_vtable *fn_ptr, const struct mv *center_mv";
specialize qw/vp10_diamond_search_sad/;

add_proto qw/int vp10_full_range_search/, "const struct macroblock *x, const struct search_site_config *cfg, struct mv *ref_mv, struct mv *best_mv, int search_param, int sad_per_bit, int *num00, const struct vpx_variance_vtable *fn_ptr, const struct mv *center_mv";
specialize qw/vp10_full_range_search/;

add_proto qw/void vp10_temporal_filter_apply/, "uint8_t *frame1, unsigned int stride, uint8_t *frame2, unsigned int block_width, unsigned int block_height, int strength, int filter_weight, unsigned int *accumulator, uint16_t *count";
specialize qw/vp10_temporal_filter_apply sse2 msa/;

if (vpx_config("CONFIG_VPX_HIGHBITDEPTH") eq "yes") {

  # ENCODEMB INVOKE
  if (vpx_config("CONFIG_NEW_QUANT") eq "yes") {
    add_proto qw/void highbd_quantize_nuq/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, const int16_t *dequant_ptr, const cuml_bins_type_nuq *cuml_bins_ptr, const dequant_val_type_nuq *dequant_val, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, uint16_t *eob_ptr, const int16_t *scan, const uint8_t *band";
    specialize qw/highbd_quantize_nuq/;

    add_proto qw/void highbd_quantize_fp_nuq/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *quant_ptr, const int16_t *dequant_ptr, const cuml_bins_type_nuq *cuml_bins_ptr, const dequant_val_type_nuq *dequant_val, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, uint16_t *eob_ptr, const int16_t *scan, const uint8_t *band";
    specialize qw/highbd_quantize_fp_nuq/;

    add_proto qw/void highbd_quantize_32x32_nuq/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, const int16_t *dequant_ptr, const cuml_bins_type_nuq *cuml_bins_ptr, const dequant_val_type_nuq *dequant_val, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, uint16_t *eob_ptr, const int16_t *scan, const uint8_t *band";
    specialize qw/highbd_quantize_32x32_nuq/;

    add_proto qw/void highbd_quantize_32x32_fp_nuq/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *quant_ptr, const int16_t *dequant_ptr, const cuml_bins_type_nuq *cuml_bins_ptr, const dequant_val_type_nuq *dequant_val, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, uint16_t *eob_ptr, const int16_t *scan, const uint8_t *band";
    specialize qw/highbd_quantize_32x32_fp_nuq/;
  }

  add_proto qw/int64_t vp10_highbd_block_error/, "const tran_low_t *coeff, const tran_low_t *dqcoeff, intptr_t block_size, int64_t *ssz, int bd";
  specialize qw/vp10_highbd_block_error sse2/;

  add_proto qw/void vp10_highbd_quantize_fp/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan, int log_scale";
  specialize qw/vp10_highbd_quantize_fp sse4_1/;

  add_proto qw/void vp10_highbd_quantize_b/, "const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan, int log_scale";
  specialize qw/vp10_highbd_quantize_b/;

  # fdct functions
  add_proto qw/void vp10_highbd_fht4x4/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_highbd_fht4x4 sse4_1/;

  add_proto qw/void vp10_highbd_fht8x4/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_highbd_fht8x4/;

  add_proto qw/void vp10_highbd_fht4x8/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_highbd_fht4x8/;

  add_proto qw/void vp10_highbd_fht8x8/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_highbd_fht8x8/;

  add_proto qw/void vp10_highbd_fht16x16/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_highbd_fht16x16/;

  add_proto qw/void vp10_highbd_fht32x32/, "const int16_t *input, tran_low_t *output, int stride, int tx_type";
  specialize qw/vp10_highbd_fht32x32/;

  add_proto qw/void vp10_highbd_fwht4x4/, "const int16_t *input, tran_low_t *output, int stride";
  specialize qw/vp10_highbd_fwht4x4/;

  add_proto qw/void vp10_highbd_temporal_filter_apply/, "uint8_t *frame1, unsigned int stride, uint8_t *frame2, unsigned int block_width, unsigned int block_height, int strength, int filter_weight, unsigned int *accumulator, uint16_t *count";
  specialize qw/vp10_highbd_temporal_filter_apply/;

}
# End vp10_high encoder functions

if (vpx_config("CONFIG_EXT_INTER") eq "yes") {
  add_proto qw/uint64_t vp10_wedge_sse_from_residuals/, "const int16_t *r1, const int16_t *d, const uint8_t *m, int N";
  specialize qw/vp10_wedge_sse_from_residuals sse2/;
  add_proto qw/int vp10_wedge_sign_from_residuals/, "const int16_t *ds, const uint8_t *m, int N, int64_t limit";
  specialize qw/vp10_wedge_sign_from_residuals sse2/;
  add_proto qw/void vp10_wedge_compute_delta_squares/, "int16_t *d, const int16_t *a, const int16_t *b, int N";
  specialize qw/vp10_wedge_compute_delta_squares sse2/;
}

}
# end encoder functions
1;
