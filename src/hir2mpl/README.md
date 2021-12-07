```
#
# Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#
```

hir2mpl supports ast, dex, and jbc as inputs.

## Building hir2mpl

source build/envsetup.sh arm release/debug

make hir2mpl

## Usage

hir2mpl -h to view available options

example for ast

${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang -emit-ast test.c -o test.ast

$MAPLE_ROOT/output/aarch64-clang-release/bin/hir2mpl test.ast -o test.mpl or $MAPLE_ROOT/output/aarch64-clang-debug/bin/hir2mpl test.ast -o test.mpl
