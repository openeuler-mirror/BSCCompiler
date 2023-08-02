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

REFINECATCH = {
    "compile": [
        Smali2dex(
            file=["${APP}.smali"]
        ),
        Dex2mpl(
            dex2mpl="${OUT_ROOT}/target/product/maple_arm64/bin/dex2mpl",
            option="--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-O2/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/maple_arm64/lib/codetricks/profile.pv/meta.list -refine-catch -dumpdataflow -func=${FUNC}",
            infile="${APP}.dex",
            redirection="dex2mpl.log"
        )
    ]
}