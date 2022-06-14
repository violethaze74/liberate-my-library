/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */
#ifndef AV1_COMMON_CLPF_H_
#define AV1_COMMON_CLPF_H_

#include "av1/common/reconinter.h"

#define MAX_FB_SIZE_LOG2 7
#define MIN_FB_SIZE_LOG2 5
#define MAX_FB_SIZE (1 << MAX_FB_SIZE_LOG2)
#define MIN_FB_SIZE (1 << MIN_FB_SIZE_LOG2)

int av1_clpf_sample(int X, int A, int B, int C, int D, int E, int F, int b);
void av1_clpf_frame(const YV12_BUFFER_CONFIG *frame,
                    const YV12_BUFFER_CONFIG *org, AV1_COMMON *cm,
                    int enable_fb_flag, unsigned int strength,
                    unsigned int fb_size_log2, int plane,
                    int (*decision)(int, int, const YV12_BUFFER_CONFIG *,
                                    const YV12_BUFFER_CONFIG *,
                                    const AV1_COMMON *cm, int, int, int,
                                    unsigned int, unsigned int, int8_t *));

#endif
