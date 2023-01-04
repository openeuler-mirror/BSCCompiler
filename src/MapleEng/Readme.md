```
#
# Copyright (C) [2022] Futurewei Technologies, Inc. All rights reserved.
#
# OpenArkCompiler is licensed underthe Mulan Permissive Software License v2.
# You can use this software according to the terms and conditions of the MulanPSL - 2.0.
# You may obtain a copy of MulanPSL - 2.0 at:
#
#   https://opensource.org/licenses/MulanPSL-2.0
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the MulanPSL - 2.0 for more details.
#
```
## Maple Engine for Lmbc
  Maple Engine for Lmbc is an interpreter that executes Lmbc files (.lmbc) that the
  OpenArkCompiler generates. C source code is first parsed by C to Maple frontends, which
  can be either hir2mpl or clang2mpl, and the output is then compiled by OpernArkCompiler
  into .lmbc (lowered Maple bytecode format) format to be executed by Maple Engine for Lmbc.

## Build OpernArkCompiler and engine 
  The following build example assumes OpenArkCompiler root directory at ~/OpenArkCompiler:
```
  cd ~/OpenArkCompiler
  source build/envsetup.sh arm release
  make
  make clang2mpl
  make mplsh_lmbc
```
## Build and run a C app
  The following example compiles a C demo program at ~/OpenArkCompiler/test/c_demo/ to lmbc
  and runs it with Maple Engine for Lmbc.
```
  cd $MAPLE_ROOT/test/c_demo
  $MAPLE_ROOT/src/MapleEng/lmbc/test/c2lmbc.sh printHuawei.c
  $MAPLE_EXECUTE_BIN/mplsh-lmbc printHuawei.lmbc
```
## Running ctorture with Maple Engine for Lmbc
```
  git clone https://gitee.com/hu-_-wen/ctorture
  cd ctorture
  ./mpleng.sh mpleng.list
``` 
