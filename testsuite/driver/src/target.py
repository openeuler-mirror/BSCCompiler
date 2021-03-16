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
import re

from env_var import EnvVar

class Target(object):

    case_regx = re.compile('^[A-Z]{1,9}[0-9]{3,10}-[a-zA-Z0-9_.]')
    module_regx = re.compile('^[a-z0-9_]{1,20}_test$')

    def __init__(self, target):
        self.target = target
        self.cases = set()

    def get_cases(self):
        if Target.case_regx.match(self.target.split('/')[-1]):
            self.cases.add(self.target)
        elif Target.module_regx.match(self.target.split('/')[-1]):
            subtarget_list = [self.target]
            while subtarget_list:
                subtarget = subtarget_list.pop(0)
                for dir in os.listdir(os.path.join(EnvVar.TEST_SUITE_ROOT, subtarget)):
                    if Target.case_regx.match(dir):
                        self.cases.add(os.path.join(subtarget, dir))
                    elif Target.module_regx.match(dir):
                        subtarget_list.append(os.path.join(subtarget, dir))
        return self.cases
