#!/bin/bash

# usage: in OpenArkCompiler dir, ./format.sh xxx.cpp
ROOT_DIR=$(cd "$(dirname $0)"; pwd)
CLANG_FORMAT=$ROOT/tools/clang+llvm-15.0.4-x86_64-linux-gnu-ubuntu-18.04-enhanced/bin/clang-format

$CLANG_FORMAT -style=file -i $1
sed -i -e 's/ \*,/\*,/g' -e 's/ \*>/\*>/g' -e 's/ \*)/\*)/g'  -e 's/ \&,/\&,/g' -e 's/ \&>/\&>/g' -e 's/ \&)/\&)/g' $1
sed -i ":a;$!N;s/enum\(.*\)\n{/enum\1 {/g;ba" $1
