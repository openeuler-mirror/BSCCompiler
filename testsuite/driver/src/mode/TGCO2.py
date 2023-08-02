#
# Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

from api import *

TGCO2 = {
    "compile": [
        Shell(
            "adb shell \"mkdir -p /data/maple/${CASE}/${OPT}\""
        ),
        Java2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java","${EXTRA_JAVA_FILE}"]
        ),
        Shell(
            "adb push ${APP}.dex /data/maple/${CASE}/${OPT}/"
        ),
        Shell(
            "adb shell \"/data/maple/maple -O2 --gconly --save-temps --hir2mpl-opt=\\\"-Xbootclasspath /apex/com.android.runtime/javalib/core-oj.jar,/apex/com.android.runtime/javalib/core-libart.jar\\\" --mplcg-opt=\\\"--no-ebo --no-cfgo --no-schedule\\\" --infile /data/maple/${CASE}/${OPT}/${APP}.dex\""
        ),
        Shell(
            "adb pull /data/maple/${CASE}/${OPT}/${APP}.VtableImpl.s ./"
        ),
        Shell(
            "${MAPLE_ROOT}/zeiss/prebuilt/sdk/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang++ -O2 -x assembler-with-cpp -march=armv8-a -DUSE_32BIT_REF -c ${APP}.VtableImpl.s"
        ),
        Shell(
            "${MAPLE_ROOT}/zeiss/prebuilt/sdk/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang++ ${APP}.VtableImpl.o -O2 -Wall -Werror -Wno-unused-command-line-argument -fstack-protector-strong -std=c++14 -nostdlibinc -march=armv8-a -fPIC -shared ${MAPLE_ROOT}/out/target/product/maple_arm64/lib/mrt_module_init.cpp -fuse-ld=lld -rdynamic -L${MAPLE_ROOT}/out/target/product/maple_arm64/lib/android -lmaplecore-all -lcommon_bridge -lc++ -lc -lm -ldl -Wl,-T${MAPLE_ROOT}/out/target/product/public/lib/linker/maplelld.so.lds -o ./${APP}.so"
        ),
        Shell(
            "adb push ${APP}.so /data/maple/${CASE}/${OPT}/"
        )
    ],
    "run": [
        Shell(
            "adb shell \"export LD_LIBRARY_PATH=/vendor/lib64:/system/lib64:/data/maple;mplsh -Xgconly -cp /data/maple/${CASE}/${OPT}/${APP}.so ${APP}\" > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        ),
        Shell(
            "adb shell \"rm -rf /data/maple/${CASE}/${OPT}\""
        )
    ]
}