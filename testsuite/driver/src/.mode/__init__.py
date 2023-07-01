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

mode_dict = {}
my_dir = os.path.dirname(__file__)
for py in os.listdir(my_dir):
    if py == '__init__.py':
        continue

    if py.endswith('.py'):
        name = py[:-3]
        mode = __import__(__name__, globals(), locals(), ['%s' % name])
        mode_dict[name] = getattr(getattr(mode, name), name)
