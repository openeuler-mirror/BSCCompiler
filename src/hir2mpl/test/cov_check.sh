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
CORE_OJ_JAR=${MAPLE_ROOT}/android/emui-10.0/out/target/common/obj/JAVA_LIBRARIES/core-oj_intermediates/classes.jar
CORE_LIBART_JAR=${MAPLE_ROOT}/android/emui-10.0/out/target/common/obj/JAVA_LIBRARIES/core-libart_intermediates/classes.jar

rm -rf ${MAPLE_ROOT}/report
${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT ext -gen-base64 xxx.file
${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT ext -gen-base64 ${MAPLE_ROOT}/hir2mpl/test/jbc_input/JBC0001/Test.class
${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT ext -in-class xxx.class
${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT ext -in-class ${MAPLE_ROOT}/hir2mpl/test/jbc_input/JBC0001/Test.class
${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT ext -in-jar xxx.jar
${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT ext -in-jar ${CORE_OJ_JAR},${CORE_LIBART_JAR}
${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT ext -mplt ${MAPLE_ROOT}/out/target/product/maple_arm64-clang-release/lib/host-x86_64-HIR2MPL_DEXO0/libcore-all.mplt
# ${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT test
${MAPLE_ROOT}/out/target/product/maple_arm64/bin/hir2mplUT testWithMplt ${MAPLE_ROOT}/out/target/product/maple_arm64-clang-release/lib/host-x86_64-HIR2MPL_DEXO0/libcore-all.mplt
bash ${MAPLE_ROOT}/zeiss/prebuilt/tools/coverage_check/coverage_check.sh hir2mpl

