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
set -e

str_num=$(ls | grep ^SIM | tail -n 1 | cut -d "-" -f 1 | cut -d "M" -f 2)
let str_num++
str_num_next=$(printf "%05d\n" ${str_num})
next_name=SIM${str_num_next}-${1}
mkdir ${next_name}
cd ${next_name}
echo "clean()" >> test.cfg
echo "compile(${2})" >> test.cfg
echo "run(${2})" >> test.cfg
echo "#include <stdio.h>" >> ${2}.c
echo "" >> ${2}.c
echo "int main(){" >> ${2}.c
echo "" >> ${2}.c
echo "    return 0;" >> ${2}.c
echo "}" >> ${2}.c
