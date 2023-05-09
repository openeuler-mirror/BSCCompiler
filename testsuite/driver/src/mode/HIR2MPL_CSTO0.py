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

HIR2MPL_CSTO0 = {
    "compile": [
        Shell(
            "cp ${OUT_ROOT}/target/product/public/lib/libcore-all.dex ."
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/bin_HIR2MPL/maple",
            run=["hir2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "hir2mpl": "",
                "me": "--quiet --ignore-inferred-ret-type --gconly",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --maplelinker-nolocal",
                "mplcg": "--quiet --no-pie --verbose-asm --gen-c-macro-def --gconly --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/public/lib/codetricks/arch/arm64/duplicateFunc.s --fPIC"
            },
            global_option="--save-temps",
            infile="${APP}.dex"
        ),
        Shell(
            "mv ${APP}.mpl serial_${APP}.mpl;"
            "mv ${APP}.VtableImpl.mpl serial_${APP}.VtableImpl.mpl;"
            "mv ${APP}.VtableImpl.s serial_${APP}.VtableImpl.s"
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/bin_HIR2MPL/maple",
            run=["hir2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "hir2mpl": "--np 4",
                "me": "--quiet --threads=4 --gconly",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --maplelinker-nolocal",
                "mplcg": "--quiet --no-pie --verbose-asm --gen-c-macro-def --gconly --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/public/lib/codetricks/arch/arm64/duplicateFunc.s --fPIC"
            },
            global_option="--save-temps",
            infile="${APP}.dex"
        ),
        Shell(
            "mv ${APP}.mpl parallel_${APP}.mpl;"
            "mv ${APP}.VtableImpl.mpl parallel_${APP}.VtableImpl.mpl;"
            "mv ${APP}.VtableImpl.s parallel_${APP}.VtableImpl.s"
        )
    ],
    "check": [
        Shell(
            "diff serial_${APP}.mpl parallel_${APP}.mpl"
        ),
        Shell(
            "diff serial_${APP}.VtableImpl.mpl parallel_${APP}.VtableImpl.mpl"
        ),
        Shell(
            "diff serial_${APP}.VtableImpl.s parallel_${APP}.VtableImpl.s"
        )
    ]
}
