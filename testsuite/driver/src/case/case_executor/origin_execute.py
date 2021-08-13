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

from case.component import Component
from env_var import EnvVar
from case.case_executor.command_executor.shell import Shell


class OriginExecute(Component):

    def __init__(self, input: dict):
        self.case_name = input["case_name"]
        self.command_suite = input["command_suite"]
        self.detail = input["detail"]
        self.result_suite = {"PASSED": set(), "FAILED": set(), "TIMEOUT": set()}

    def execute(self):
        for mode in self.command_suite.keys():
            case = SingleCaseExecutor(self.case_name, mode, self.command_suite[mode], self.detail)
            case.execute()
            result = case.get_result()
            self.result_suite[result].add(mode)

    def get_output(self):
        return {self.case_name: self.result_suite}

class SingleCaseExecutor(object):

    def __init__(self, case_name: str, mode: str, command_list: list, detail: bool):
        self.case_name = case_name
        self.mode = mode
        self.command_list = command_list
        self.detail = detail
        self.case_path = os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name)
        self.result = ""

    def execute(self):
        passed, passed_in_color = "PASSED", "\033[1;32mPASSED\033[0m"
        failed, failed_in_color = "FAILED", "\033[1;31mFAILED\033[0m"
        timeout, timeout_in_color = "TIMEOUT", "\033[1;33mTIMEOUT\033[0m"
        self.result, result_in_color = passed, passed_in_color
        if self.detail:
            for command in self.command_list:
                print("\033[1;32m[[ CMD : " + command + " ]]\033[0m")
                exe = Shell(command=command, workdir=self.case_path, timeout=5000)
                exe.execute()
                com_out, com_err, return_code = exe.get_output()
                if com_out is not None and len(com_out) != 0:
                    print(com_out)
                if com_err is not None and len(com_err) != 0:
                    print(com_err)
                if return_code == 124:
                    print("ERROR : TIMEOUT !")
                    self.result, result_in_color = timeout, timeout_in_color
                    break
                elif return_code != 0:
                    print("ERROR : FAILED !")
                    self.result, result_in_color = failed, failed_in_color
                    break
        else:
            log_file = open(os.path.join(self.case_path, self.mode + "_run.log"), "w+")
            for command in self.command_list:
                log_file.write("[[ CMD : " + command + " ]]\n")
                exe = Shell(command=command, workdir=self.case_path, timeout=5000)
                exe.execute()
                com_out, com_err, return_code = exe.get_output()
                if com_out is not None and len(com_out) != 0:
                    log_file.write(com_out + "\n")
                if com_err is not None and len(com_err) != 0:
                    log_file.write(com_err + "\n")
                if return_code == 124:
                    log_file.write("ERROR : TIMEOUT !\n")
                    self.result, result_in_color = timeout, timeout_in_color
                    break
                elif exe.return_code != 0:
                    log_file.write("ERROR : FAILED !\n")
                    self.result, result_in_color = failed, failed_in_color
                    break
        print(self.case_name + " " + self.mode + " " + result_in_color)
        sys.stdout.flush()

    def get_result(self):
        return self.result
