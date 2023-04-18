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

DRIVER = {
    "compile": [
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="${OPTION}",
            inputs="${APP}"
        )
    ],
    "compileWithGcc": [
        Driver(
            maple="${OUT_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc ",
            global_option="${OPTION}",
            inputs="${APP}"
        )
    ],
    "mpl2S2out": [
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="-S -o ${CPP}",
            inputs="${APP}"
        ),
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="",
            inputs="${CPP}"
        )
    ],
    "mpl2S2outNo": [
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="-S",
            inputs="${APP}"
        ),
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="",
            inputs="${CPP}"
        )
    ],
    "mpl2O2out": [
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="-c -o ${BPP}",
            inputs="${APP}"
        ),
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="",
            inputs="${BPP}"
        )
    ],
    "mpl2O2outNo": [
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="-c",
            inputs="${APP}"
        ),
        Driver(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            global_option="",
            inputs="${BPP}"
        )
    ],
    "run": [
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${APP}.out > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
