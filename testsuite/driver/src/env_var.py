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

import os
import sys

class EnvVar(object):

    SDK_ROOT = None
    TEST_SUITE_ROOT = None
    SOURCE_CODE_ROOT = None
    MAPLE_BUILD_TYPE = None
    #TODO:Delete
    CONFIG_FILE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "config")

    def __init__(self, sdk_root, test_suite_root, source_code_root):
        if sdk_root is None:
            if "OUT_ROOT" in os.environ.keys():
                EnvVar.SDK_ROOT = os.environ["OUT_ROOT"]
            else:
                print("Please source zeiss/envsetup.sh")
                sys.exit(1)
        else:
            EnvVar.SDK_ROOT = sdk_root
        if test_suite_root is None:
            if "CASE_ROOT" in os.environ.keys():
                EnvVar.TEST_SUITE_ROOT = os.environ["CASE_ROOT"]
            else:
                print("Please source zeiss/envsetup.sh")
                sys.exit(1)
        else:
            EnvVar.TEST_SUITE_ROOT = test_suite_root
        if source_code_root is None:
            if "MAPLE_ROOT" in os.environ.keys():
                EnvVar.SOURCE_CODE_ROOT = os.environ["MAPLE_ROOT"]
            else:
                print("Please source zeiss/envsetup.sh")
                sys.exit(1)
        else:
            EnvVar.SOURCE_CODE_ROOT = source_code_root
        if "MAPLE_BUILD_TYPE" in os.environ.keys():
            EnvVar.MAPLE_BUILD_TYPE = os.environ["MAPLE_BUILD_TYPE"]
