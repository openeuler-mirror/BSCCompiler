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

DEX = {
    "compile": [
        Dex2mpl(
            dex2mpl="${OUT_ROOT}/target/product/maple_arm64/bin/dex2mpl",
            option="--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-O0/libcore-all.mplt -litprofile=/home/fuqun/maple3.0/out/target/product/maple_arm64/lib/codetricks/profile.pv/meta.list",
            infile="${APP}.dex"
        ),
        Irbuild(
            irbuild="${OUT_ROOT}/target/product/maple_arm64/bin/irbuild",
            infile="${APP}.mpl"
        ),
        Mplverf(
            mplverf="${OUT_ROOT}/target/product/maple_arm64/bin/mplverf",
            infile="${APP}.mpl"
        ),
        Mplme(
            mplme="${OUT_ROOT}/target/product/maple_arm64/bin/mplme",
            option="-O2",
            infile="${APP}.mpl"
        )
    ],
    "run": []
}