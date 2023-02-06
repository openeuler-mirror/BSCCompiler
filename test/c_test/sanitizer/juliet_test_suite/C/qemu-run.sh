#!/bin/bash

gcc_linaro_lib="$MAPLE_ROOT/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/lib64"
LD_PRELOAD="$gcc_linaro_lib/libasan.so"
bin_path=$1
cmd="ASAN_OPTIONS=detect_leaks=0 qemu-aarch64 -L /usr/aarch64-linux-gnu -E LD_PRELOAD=$LD_PRELOAD -E LD_LIBRARY_PATH=$gcc_linaro_lib $bin_path"

eval $cmd

