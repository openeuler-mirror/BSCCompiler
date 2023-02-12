#!/bin/bash

cmd="FORCE_UNSAFE_CONFIGURE=1 ./configure --prefix=$(pwd)/../clang-install --without-selinux CC=/root/git/ThirdParty/llvm-12.0.0-personal/build/bin/clang CFLAGS='-fsanitize=address -O2 -g -DGNULIB_defined_struct_option -fno-inline'"

echo $cmd
eval $cmd


