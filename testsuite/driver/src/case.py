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
import stat
import sys

from api.shell import Shell
from test_cfg import TestCFG
from env_var import EnvVar
from mode import *
from shell_executor import ShellExecutor


class Case(object):

    def __init__(self, case_name, modes):
        self.case_name = case_name
        self.modes = modes
        self.case_timeout = 900
        self.case_cpu = 0.7
        self.case_memory = 0
        self.test_cfg = TestCFG(os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name, "test.cfg"))
        self.test_cfg_content = self.test_cfg.test_cfg
        self.shell_command_suite = {}

    def parse_config_file(self):
        for mode in self.modes:
            self.shell_command_suite[mode] = []
            global_variables = {"CASE": self.case_name, "MODE": mode}
            if mode in self.test_cfg_content.keys():
                mode_cfg_content = self.test_cfg_content[mode]
            else:
                mode_cfg_content = self.test_cfg_content["default"]
            for line in mode_cfg_content:
                if line[0] == "global":
                    global_variables.update(line[1])
                elif line[0] == "shell":
                    self.shell_command_suite[mode].append(Shell(line[1]).get_command(global_variables))
                else:
                    local_variables = line[1]
                    variables = global_variables.copy()
                    variables.update(local_variables)
                    phase = line[0]
                    if phase in globals()[mode].keys():
                        for command_object in globals()[mode][phase]:
                            self.shell_command_suite[mode].append(command_object.get_command(variables))
                    else:
                        print(line[0] + " not found !")
                        os._exit(1)

    def generate_test_script(self):
        for mode in self.modes:
            mode_file_path = os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name, "." + mode + "_test.sh")
            f = open(mode_file_path, 'w')
            f.write("#!/bin/bash\n")
            f.write("set -e\n")
            f.write("set -x\n")
            for line in self.shell_command_suite[mode]:
                f.write(line + '\n')
            f.close()
            os.chmode(mode_file_path, stat.S_IRWXU)

    def clean(self):
        os.chdir(os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name))
        for mode in self.shell_command_suite.keys():
            for command in self.shell_command_suite[mode]:
                if command.startswith("rm "):
                    os.system(command)

    def get_case_package(self):
        case_list = []
        for mode in self.modes:
            case_list.append({
                "name": self.case_name,
                "memory": self.case_memory,
                "cpu": self.case_cpu,
                "option": mode,
                "cmd": "cd ${WORKSPACE}/out/host/test/" + self.case_name + ";export OUT_ROOT=${WORKSPACE}/out;bash ." + mode + "_test.sh",
                "timeout": self.case_timeout
            })
        return case_list

    def run(self, detail = False):
        result_table = {}
        for mode in self.modes:
            result, result_in_color = "PASSED", "\033[1;32mPASSED\033[0m"
            log_file = open(os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name, mode + "_run.log"), "w+")
            for command in self.shell_command_suite[mode]:
                log_file.write("[[ CMD : " + command + " ]]\n")
                if detail:
                    print("\033[1;32m[[ CMD : " + command + " ]]\033[0m")
                exe = ShellExecutor(command=command, workdir=os.path.join(EnvVar.TEST_SUITE_ROOT, self.case_name), timeout=300)
                exe.shell_execute()
                if exe.com_out is not None and len(exe.com_out) != 0:
                    if detail:
                        print(exe.com_out)
                    log_file.write(exe.com_out + "\n")
                if exe.com_err is not None and len(exe.com_err) != 0:
                    if detail:
                        print(exe.com_err)
                    log_file.write(exe.com_err + "\n")
                if exe.return_code == 124:
                    if detail:
                        print("ERROR : TIMEOUT !")
                    log_file.write("ERROR : TIMEOUT !\n")
                    result, result_in_color = "TIMEOUT", "\033[1;33mTIMEOUT\033[0m"
                    break
                elif exe.return_code != 0:
                    if detail:
                        print("ERROR : FAILED !")
                    log_file.write("ERROR : FAILED !\n")
                    result, result_in_color = "FAILED", "\033[1;31mFAILED\033[0m"
                    break
            print(self.case_name + " " + mode + " " + result_in_color)
            sys.stdout.flush()
            result_table[mode] = result
        return {self.case_name: result_table}