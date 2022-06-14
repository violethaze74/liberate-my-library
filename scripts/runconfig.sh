#!/bin/sh

platform=x86_64-linux-gcc
codec=--enable-av1
libsrc=aom
script_dir=~/Dev/sandbox/libvpx/scripts

experiment_dependency() {
  tool=--enable-$1

  if [ "$1" == ec_adapt ]; then
    tool="--enable-$1 --enable-ext-inter"
  fi

  if [ "$1" == ec_multisymbol ]; then
    tool="--enable-$1 --enable-daala-ec"
  fi

  if [ "$1" == rect_tx ]; then
    tool="--enable-$1 --enable-ext-tx"
  fi

  if [ "$1" == simp_mv_pred ]; then
    tool="--enable-$1 --enable-ref-mv"
  fi
  
  if [ "$1" == mv_compress ]; then
    tool="--enable-$1 --enable-ref-mv"
  fi

  if [ "$1" == var_tx ]; then
    tool="--enable-$1 --enable-cb4x4 --enable-rect-tx"
  fi
  
  # if [ "$1" == ext_tx ]; then
  #   tool="--enable-$1 --enable-cb4x4"
  # fi
  
  # if [ "$1" == chroma_2x2 ]; then
  #   tool="--enable-$1 --enable-cb4x4"
  # fi

  if [ "$1" == ncobmc ]; then
    tool="--enable-$1 --enable-motion-var"
  fi
  
  if [ "$1" == wedge ] || [ "$1" == compound_segment ] ; then
    tool="--enable-$1 --enable-ext-inter"
  fi

  if [ "$1" == interintra ] ; then
    tool="--enable-$1 --enable-ext-inter --enable-wedge"
  fi

  if [ "$1" == txk_sel ] ; then
    tool="--enable-$1 --enable-lv_map"
  fi

  if [ "$1" == intra_interp ] ; then
    tool="--enable-$1 --enable-ext-intra"
  fi

  if [ "$1" == ext_tile ] ; then
    tool="--enable-$1 --disable-cdef"
  fi

  if [ "$1" == lv_map ] ; then
    tool="--enable-$1 --enable-cb4x4 --enable-chroma_sub8x8"
  fi

  # if [ "$1" == chroma_sub8x8 ] ; then
  #   tool="--enable-$1 --enable-cb4x4"
  # fi

  if [ "$1" == one_sided_compound ] ; then
    tool="--enable-$1 --enable-ext-refs"
  fi
  
}

#debug="--enable-debug --disable-optimizations"
common="--disable-unit-tests --disable-docs --enable-experimental"

. $script_dir/disabled_list.sh

experiment_dependency $1

../$libsrc/configure $common $debug $disabled $tool > /dev/null
