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

#ifndef AOM_DSP_AOM_AOM_SIMD_H_
#define AOM_DSP_AOM_AOM_SIMD_H_

#include <stdint.h>

#if defined(_WIN32)
#include <intrin.h>
#endif

#include "./aom_config.h"
#include "./aom_simd_inline.h"

#if HAVE_NEON
#include "simd/v256_intrinsics_arm.h"
#elif HAVE_SSE2
#include "simd/v256_intrinsics_x86.h"
#else
#include "simd/v256_intrinsics.h"
#endif

#endif  // AOM_DSP_AOM_AOM_SIMD_H_
