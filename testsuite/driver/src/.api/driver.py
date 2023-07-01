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


class Driver(ShellOperator):

    def __init__(self, maple, global_option, inputs, return_value_list=None, redirection=None):
        super().__init__(return_value_list, redirection)
        self.maple = maple
        self.global_option = global_option
        self.inputs = inputs

    def get_command(self, variables):
        self.command = self.maple + " "
        self.command += self.global_option + " "
        self.command += self.inputs + " "

        return super().get_final_command(variables)
