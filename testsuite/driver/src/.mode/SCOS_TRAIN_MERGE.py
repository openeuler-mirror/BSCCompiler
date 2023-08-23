#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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


SCOS_TRAIN_MERGE = {
    "c2o": [
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}.c"],
            outfile="${TARGET}",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include"
            ],
            option="--Os --patch-long-branch -fPIC --no-pie -std=gnu99 -lm -L${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib/",
            extra_opt="${SPEC_PARAM}"
        )
    ],
    "compile": [
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}.c"],
            outfile="${APP}.o",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include"
            ],
            option="--Os -fPIC -g --no-pie -flto -c",
            extra_opt="${SPEC_PARAM}"
        )
    ],
    "link": [
        MapleDriver(
            maple="${OUT_ROOT}/aarch64-clang-release/bin/maple",
            infiles=["${APP}"],
            outfile="${TARGET}",
            option="-std=gnu99 --no-pie -lm -L${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib/",
            extra_opt="-Os ${SPEC_PARAM}"
        )
    ],
    "cp_data":[
        Shell(
            "cp -r data/train/${APP} ${TARGET}"
        )
    ],
    "run": [
        Shell(
            "${MAPLE_ROOT}/tools/bin/qemu-aarch64 -L ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${EXE} ${APP} > output.log"
        )
    ],
    "compare": [
        Shell(
            "${MAPLE_ROOT}/testsuite/c_test/spec_test/specperl ${MAPLE_ROOT}/testsuite/c_test/spec_test/specdiff -m -l 10 ${EXTRA_COMPARE} output.log data/train/${APP}"
        )
    ]
}
