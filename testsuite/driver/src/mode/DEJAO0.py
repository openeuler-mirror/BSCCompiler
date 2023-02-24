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

DEJAO0 = {
    "prepare": [
        Shell("cp -r ../lib . && cp -r ../site.exp .")
    ],
    "compile": [
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}.c"],
            outfile="${APP}.out",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib"
            ],
            option="-O0 -I../h -I../lib -g -fPIC --no-pie -std=gnu99 -L../lib/lib -lm",
        )
    ],
    "run": [
        Shell("runtest -V || (echo 'pls install dejagnu first!' && exit 1)"),
        Shell(
            "runtest --log_dialog ${APP} > output.log 2>&1"
        ),
        Shell("grep 'untested testcases' output.log && exit 1 || exit 0"),
        Shell("grep 'unexpected failure' output.log && exit 1 || exit 0"),
        Shell("grep 'expected passes' output.log")
    ]
}
