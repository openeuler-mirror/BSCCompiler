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

Hir2mpl supports .ast, .dex, .class and .jar as inputs. Currently, .class and .jar are not enabled.
hir2mpl enables the corresponding compilation process based on the inputs.

## Building hir2mpl

source build/envsetup.sh arm release/debug

make hir2mpl

## Usage

hir2mpl -h to view available options

## Compile .dex

First, use java2dex to generate the required xxx.dex file from xxx.java.

bash ${MAPLE_ROOT}/tools/bin/java2dex  -o xxx.dex -p <classpath> -i xxx.java

If the xxx.dex file depends on other files, use the -mplt command to add the depended file.

the depended mplts need to compile firstly. Also, you can use the -Xbootclasspath to

load the dependent JAR package.

${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl -mplt <classpath mplt> xxx.dex -o xxx.mpl

${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl -Xbootclasspath=<classpath> xxx.dex -o xxx.mpl

Note:
1. The GC mode is used for memory management by default.
   If you want to use the RC mode, you can use the "-rc" option to switch.
2. Invalid option scope "ast Compile Options" and "Security Check".

## Compile .ast

First, use clang to generate the required xxx.ast file from xxx.c.

${MAPLE_ROOT}/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/clang -emit-ast xxx.c -o xxx.ast

${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl xxx.ast -o xxx.mpl

Note:
1. The backend supports variable arrays that are incomplete.
   By default, the frontend intercepts variable arrays.
   You can use the "-enable-variable-array" option to enable the frontend to support variable arrays.

2. hir2mpl supports static security check and check statement insertion based on source code annotation.
   However, the source code must be compiled using the Clang binary provided by the Maple community.
   You can enable the corresponding check by selecting the corresponding option "-npe-check-dynamic"
   and "-boundary-check-dynamic".

3. Invalid option scope "BC Bytecode Compile Options" and "On Demand Type Creation".

## UT Test

Building UT target : make hir2mplUT

Before running the UT test, compile the libcore which the UT depends.

make libcore

bash $MAPLE_ROOT/src/hir2mpl/test/hir2mplUT_check.sh

## Directory structure

hir2mpl
├── ast_input                     # .ast/.mast parser
│   ├── clang
│   ├── common
│   └── maple
├── BUILD.gn                      # build configuration files
├── bytecode_input                # .class/.dex parser
│   ├── class
│   ├── common
│   └── dex
├── common                        # feir part
│   ├── include
│   └── src
├── optimize                      # lower/cfg/dfg(TODO Refactoring) and pattern match opt
│   ├── include
│   └── src
├── README.md
└── test                          # UT test
    ├── ast_input
    ├── BUILD.gn
    ├── bytecode_input
    ├── common
    ├── cov_check.sh
    ├── hir2mplUT_check.sh
    └── ops_ut_check.sh
