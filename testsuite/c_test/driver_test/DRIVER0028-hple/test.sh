#!/bin/bash
#
# Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

${MAPLE_BUILD_OUTPUT}/bin/maple -h > h.log
if [ $? -eq 2 ]; then
    echo "succeed"
else
    echo "failed"
fi
${MAPLE_BUILD_OUTPUT}/bin/maple --help > help.log
if [ $? -eq 2 ]; then
    echo "succeed"
else
    echo "failed"
fi
${MAPLE_BUILD_OUTPUT}/bin/maple -v
${MAPLE_BUILD_OUTPUT}/bin/maple --version
