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

IR = {
    "compile": [
        Irbuild(
            irbuild="${OUT_ROOT}/target/product/maple_arm64/bin/irbuild",
            infile="${APP}.mpl"
        ),
        Irbuild(
            irbuild="${OUT_ROOT}/target/product/maple_arm64/bin/irbuild",
            infile="${APP}.irb.mpl"
        ),
        CheckFileEqual(
            file1="${APP}.irb.mpl",
            file2="${APP}.irb.irb.mpl"
        )
    ]
}