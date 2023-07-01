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

LTOASTO0 = {
    "c2ast": [
        C2ast(
            clang="${ENHANCED_CLANG_PATH}/bin/clang",
            include_path=[
                "${OUT_ROOT}/aarch64-clang-release/lib/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include"
            ],
            option="--target=aarch64 -Wno-error=int-conversion",
            infile="${APP}.c",
            outfile="${APP}.ast",
        )
    ],
    # multiple ast input
    "lto2mpl": [
        Hir2mpl(
            hir2mpl="${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl",
            option="-wpaa",
            infile="${APP}",
            outfile="${TARGET}"
        )
    ]
}
