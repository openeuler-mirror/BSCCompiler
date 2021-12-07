#!/bin/bash
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
set -e

rm -rf ${MAPLE_ROOT}/report
${MAPLE_ROOT}/output/aarch64-clang-release/bin/hir2mplUT ext -gen-base64 ${MAPLE_ROOT}/src/hir2mpl/test/bytecode_input/class/JBC0001/Test.class
${MAPLE_ROOT}/output/aarch64-clang-release/bin/hir2mplUT ext -in-class ${MAPLE_ROOT}/src/hir2mpl/test/bytecode_input/class/JBC0001/Test.class
${MAPLE_ROOT}/output/aarch64-clang-release/bin/hir2mplUT ext -mplt ${MAPLE_ROOT}/output/aarch64-clang-release/libjava-core/host-x86_64-O2/libcore-all.mplt
${MAPLE_ROOT}/output/aarch64-clang-release/bin/hir2mplUT testWithMplt ${MAPLE_ROOT}/output/aarch64-clang-release/libjava-core/host-x86_64-O2/libcore-all.mplt


