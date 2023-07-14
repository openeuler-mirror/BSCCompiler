#!/usr/bin/env python
# coding=utf-8
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

from api.shell_operator import ShellOperator


class Linker(ShellOperator):

    def __init__(self, lib, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.lib = lib

    def get_command(self, variables):
        self.command = "${ENHANCED_CLANG_PATH}/bin/clang++ -g3 -O2 -x assembler-with-cpp -march=armv8-a -target aarch64-linux-gnu -c ${APP}.VtableImpl.s && ${ENHANCED_CLANG_PATH}/bin/clang++ ${APP}.VtableImpl.o -L${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/" + self.lib + " -g3 -O2 -march=armv8-a -target aarch64-linux-gnu -fPIC -shared -o ${APP}.so ${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/mrt_module_init.o -fuse-ld=lld -rdynamic -lcore-all -lcommon-bridge -Wl,-z,notext -Wl,-T${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/linker/maplelld.so.lds"
        return super().get_final_command(variables)
