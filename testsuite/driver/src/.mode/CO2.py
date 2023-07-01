#!/usr/bin/env python
# coding=utf-8
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

CO2 = {
    "compile": [
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}.c"],
            outfile="${APP}.out",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib/include",
                "${MAPLE_ROOT}/testsuite/c_test/csmith_test/runtime_x86"
            ],
            option="-O2 -fPIC -lpthread -lm ${option} -Wno-error=int-conversion"
        )
    ],
    "compile_err": [
        Shell("EXPECT_ERR_START"),
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}.c"],
            outfile="${APP}.out",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib/include"
            ],
            option="-O2 -fPIC -lm ${option} -g -Wno-error=int-conversion",
            redirection="compile.log"
        ),
        Shell("EXPECT_ERR_END"),
    ],
    "run": [
        Shell(
            "${MAPLE_ROOT}/tools/bin/qemu-aarch64 -L ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${APP}.out > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
