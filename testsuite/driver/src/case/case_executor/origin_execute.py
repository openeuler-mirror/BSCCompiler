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

from case.case_executor.clean import Clean
from case.case_executor.note_src import NoteSrc
from case.component import Component
from env_var import EnvVar
from case.case_executor.command_executor.shell import Shell


class OriginExecute(Component):

    def __init__(self, input: dict):
        self.case_name = input["case_name"]
        self.command_suite = input["command_suite"]
        self.detail = input["detail"]
        self.config = input["config"]
        self.timeout = input["timeout"]
        self.result_suite = {"PASSED": set(), "FAILED": set(), "TIMEOUT": set()}

    def execute(self):
        for mode in self.command_suite.keys():
            case = SingleCaseExecutor(self.case_name, mode, self.command_suite[mode], self.detail, self.config, self.timeout)
            case.execute()
            result = case.get_result()
            self.result_suite[result].add(mode)

    def get_output(self):
        return {self.case_name: self.result_suite}

class SingleCaseExecutor(object):

    def __init__(self, case_name: str, mode: str, command_list: list, detail: bool, config: dict, timeout:int):
        self.case_name = case_name
        self.mode = mode
        self.command_list = command_list
        self.detail = detail
        self.case_path = os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name)
        self.result = ""
        self.input = {'case_name': case_name, 'detail': detail}
        self.expect_err = False
        self.err_flag = False
        self.config = config
        self.timeout = timeout

    def print_and_log(self, log_file, content, is_cmd=False):
        if self.detail and not is_cmd:
            print(content)
        elif self.detail and is_cmd:
            print("\033[1;32m[[ CMD : " + content + " ]]\033[0m")
        log_file.write(content + "\n")

    def execute(self):
        TIMEOUT = self.timeout
        if "TIMEOUT" in self.config[self.mode].keys():
            TIMEOUT = int(self.config[self.mode]["TIMEOUT"])
        clean = Clean(self.input)
        clean.rm_tmp_files()
        note_src = NoteSrc(self.input)
        note_src.execute()
        passed, passed_in_color = "PASSED", "\033[1;32mPASSED\033[0m"
        failed, failed_in_color = "FAILED", "\033[1;31mFAILED\033[0m"
        timeout, timeout_in_color = "TIMEOUT", "\033[1;33mTIMEOUT\033[0m"
        self.result, result_in_color = passed, passed_in_color
        with open(os.path.join(self.case_path, self.mode + "_run.log"), "w+") as log_file:
            for command in self.command_list:
                if command == "EXPECT_ERR_START":
                    self.expect_err = True
                    continue
                elif command == "EXPECT_ERR_END":
                    if not self.err_flag:
                        self.result, result_in_color = failed, failed_in_color
                        self.print_and_log(log_file, "\033[1;31mEXPECT ERR NOT OCCUR, CASE FAIL, EXIT!\033[0m")
                        break
                    else:
                        self.err_flag = False
                        self.expect_err = False
                        continue
                elif self.err_flag:
                    continue
                self.print_and_log(log_file, command, is_cmd=True)
                exe = Shell(command=command, workdir=self.case_path, timeout=TIMEOUT)
                exe.execute()
                com_out, com_err, return_code = exe.get_output()
                if com_out is not None and len(com_out) != 0:
                    self.print_and_log(log_file, com_out)
                if com_err is not None and len(com_err) != 0:
                    self.print_and_log(log_file, com_err)
                if return_code == 124 and not self.expect_err:
                    self.print_and_log(log_file, "ERROR : TIMEOUT !")
                    self.result, result_in_color = timeout, timeout_in_color
                    break
                elif return_code != 0 and not self.expect_err:
                    self.print_and_log(log_file, "ERROR : FAILED !")
                    self.result, result_in_color = failed, failed_in_color
                    break
                elif self.expect_err and return_code != 0:
                    self.print_and_log(log_file, "E\033[1;33mXPECT ERROR OCCUR! JUMP TO EXPECT_ERR_END!\033[0m")
                    self.err_flag = True
        print(self.case_name + " " + self.mode + " " + result_in_color)
        sys.stdout.flush()

    def get_result(self):
        return self.result
