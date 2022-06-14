LIBVPX_TEST_SRCS-yes += test-data.mk

# Encoder test source
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += hantro_collage_w352h288.yuv
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += hantro_odd.yuv

LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_10_420.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_10_422.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_10_444.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_10_440.yuv
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_12_420.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_12_422.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_12_444.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_12_440.yuv
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_8_420_a10-1.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_8_420.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_8_422.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_8_444.y4m
LIBVPX_TEST_DATA-$(CONFIG_ENCODERS) += park_joy_90p_8_440.yuv

LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += desktop_credits.y4m
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += niklas_1280_720_30.y4m
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += rush_hour_444.y4m
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += screendata.y4m

LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += desktop_credits.y4m

ifeq ($(CONFIG_DECODE_PERF_TESTS),yes)
# Encode / Decode test
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += niklas_1280_720_30.yuv
# BBB VP9 streams
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-bbb_426x240_tile_1x1_180kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-bbb_640x360_tile_1x2_337kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-bbb_854x480_tile_1x2_651kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-bbb_1280x720_tile_1x4_1310kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-bbb_1920x1080_tile_1x1_2581kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-bbb_1920x1080_tile_1x4_2586kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-bbb_1920x1080_tile_1x4_fpm_2304kbps.webm
# Sintel VP9 streams
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-sintel_426x182_tile_1x1_171kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-sintel_640x272_tile_1x2_318kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-sintel_854x364_tile_1x2_621kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-sintel_1280x546_tile_1x4_1257kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-sintel_1920x818_tile_1x4_fpm_2279kbps.webm
# TOS VP9 streams
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-tos_426x178_tile_1x1_181kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-tos_640x266_tile_1x2_336kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-tos_854x356_tile_1x2_656kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-tos_854x356_tile_1x2_fpm_546kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-tos_1280x534_tile_1x4_1306kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-tos_1280x534_tile_1x4_fpm_952kbps.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-tos_1920x800_tile_1x4_fpm_2335kbps.webm
endif  # CONFIG_DECODE_PERF_TESTS

ifeq ($(CONFIG_ENCODE_PERF_TESTS),yes)
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += desktop_640_360_30.yuv
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += kirland_640_480_30.yuv
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += macmarcomoving_640_480_30.yuv
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += macmarcostationary_640_480_30.yuv
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += niklas_1280_720_30.yuv
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += niklas_640_480_30.yuv
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += tacomanarrows_640_480_30.yuv
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += tacomasmallcameramovement_640_480_30.yuv
LIBVPX_TEST_DATA-$(CONFIG_VP10_ENCODER) += thaloundeskmtg_640_480_30.yuv
endif  # CONFIG_ENCODE_PERF_TESTS

# sort and remove duplicates
LIBVPX_TEST_DATA-yes := $(sort $(LIBVPX_TEST_DATA-yes))

# VP9 dynamic resizing test (decoder)
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x180_5_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x180_5_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x180_5_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x180_5_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x180_7_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x180_7_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x180_7_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x180_7_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x240_5_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x240_5_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x240_5_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x240_5_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x240_7_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x240_7_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x240_7_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_320x240_7_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x360_5_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x360_5_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x360_5_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x360_5_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x360_7_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x360_7_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x360_7_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x360_7_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x480_5_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x480_5_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x480_5_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x480_5_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x480_7_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x480_7_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x480_7_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_640x480_7_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1280x720_5_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1280x720_5_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1280x720_5_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1280x720_5_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1280x720_7_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1280x720_7_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1280x720_7_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1280x720_7_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1920x1080_5_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1920x1080_5_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1920x1080_5_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1920x1080_5_3-4.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1920x1080_7_1-2.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1920x1080_7_1-2.webm.md5
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1920x1080_7_3-4.webm
LIBVPX_TEST_DATA-$(CONFIG_VP9_DECODER) += vp90-2-21-resize_inter_1920x1080_7_3-4.webm.md5
