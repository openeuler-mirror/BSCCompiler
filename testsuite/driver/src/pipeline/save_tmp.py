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

import sys

from case import case_pipeline


class SaveTmp(object):

    def __init__(self, input):
        self.input = input

    def save_tmp_pipeline(self):
        self.input["case_name"] = self.input["target"]
        save_tmp = case_pipeline.save_tmp_run.SaveTmpRun(self.input)
        save_tmp.execute()
        sys.exit(0)

