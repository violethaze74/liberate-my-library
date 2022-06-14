/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vpx_config.h"
#include "vpx_rtcd.h"
#include "vp8/decoder/onyxd_int.h"

void vp8_dmachine_specific_config(VP8D_COMP *pbi)
{
    /* Pure C: */
#if CONFIG_RUNTIME_CPU_DETECT
    pbi->mb.rtcd                     = &pbi->common.rtcd;
#endif

    /* Move this to common once we use it from more than one place. */
    vpx_rtcd();
}
