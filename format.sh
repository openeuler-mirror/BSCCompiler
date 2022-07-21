#!/bin/bash

# usage: in OpenArkCompiler dir, ./format.sh xxx.cpp
CLANG_FORMAT=$MAPLE_ROOT/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang-format

$CLANG_FORMAT -style=file -i $1
sed -i -e 's/ \*,/\*,/g' -e 's/ \*>/\*>/g' -e 's/ \*)/\*)/g'  -e 's/ \&,/\&,/g' -e 's/ \&>/\&>/g' -e 's/ \&)/\&)/g' $1
sed -i ":a;$!N;s/enum\(.*\)\n{/enum\1 {/g;ba" $1
