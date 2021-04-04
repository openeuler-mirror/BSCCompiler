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
import time

from multiprocessing import Pool, cpu_count, Manager
from case import Case

class Task(object):

    def __init__(self, cases):
        self.cases = cases

    def run(self):
        pass



class CleanCaseTask(Task):

    def __init__(self, case):
        super().__init__(case)
        self.case = case

    def run(self):
        self.case.parse_config_file()
        self.case.clean()

class GenShellScriptTask(Task):

    def __init__(self, cases):
        super().__init__(cases)
        self.command_pachage = []

    def run(self):
        for case in self.cases:
            case.parse_config_file()
            case.generate_test_script()
            self.command_pachage += case.get_case_package()

    def get_command_package(self):
        return self.command_pachage


class RunCaseTask(Task):

    def __init__(self, case, detail):
        super().__init__(case)
        self.case = case
        self.detail = detail

    def run(self):
        sys.stdout.flush()
        self.case.parse_config_file()
        return self.case.run(self.detail)

class RunCasesTask(Task):

    def __init__(self, cases, process_num, detail):
        super().__init__(cases)
        self.process_num = process_num
        self.detail = detail
        self.pass_num = 0
        self.fail_num = 0
        self.timeout_num = 0
        self.fail_cases_list = []
        self.timeout_cases_list = []
        self.again_cases = []
        self.results = {}

    def run(self):
        pool = Pool(min(cpu_count(), self.process_num))
        return_results = []
        for case in self.cases:
            task = RunCaseTask(case, self.detail)
            return_results.append(pool.apply_async(task.run, ()))
        finished = 0
        total = len(self.cases)
        while total != finished:
            time.sleep(1)
            finished = sum([return_result.ready() for return_result in return_results])
        self.get_results(return_results)
        pool.close()
        pool.join()

    def get_results(self, return_results: list):
        for return_result in return_results:
            self.results.update(return_result.get())

    def result_parse(self):
        for case_name in self.results.keys():
            again_mode = set()
            for mode in self.results[case_name].keys():
                if self.results[case_name][mode] == "PASSED":
                    self.pass_num += 1
                elif self.results[case_name][mode] == "FAILED":
                    self.fail_cases_list.append(case_name + " " + mode)
                    again_mode.add(mode)
                elif self.results[case_name][mode] == "TIMEOUT":
                    self.timeout_cases_list.append(case_name + " " + mode)
                    again_mode.add(mode)
            if len(again_mode) != 0:
                self.again_cases.append(Case(case_name, again_mode))
        self.fail_num = len(self.fail_cases_list)
        self.timeout_num = len(self.timeout_cases_list)
        if len(self.again_cases) == 0:
            print("============ ALL CASE RUN SUCCESSFULLY ================")
            sys.exit(0)
        print("=========== case total num : " + str(self.pass_num + self.fail_num + self.timeout_num) + "  passed num : " + str(self.pass_num) + "  failed num : " + str(self.fail_num) + "  timeout num : " + str(self.timeout_num) + " ================")
        print()
        if self.fail_num != 0:
            print("failed case list:")
            for index in self.fail_cases_list:
                print(index)
        print()
        if self.timeout_num != 0:
            print("timeout case list:")
            for index in self.timeout_cases_list:
                print(index)
        print()

    def run_again(self):
        print("========== Retry Begin =============")
        sub_task = RunCasesTask(self.again_cases, process_num=1, detail=True)
        sub_task.run()
        sub_task.result_parse()

    def gen_report(self, report):
        if len(self.again_cases) != 0:
            f = open(report, 'w')
            f.write("=========== case total num : " + str(self.pass_num + self.fail_num + self.timeout_num) + "  passed num : " + str(self.pass_num) + "  failed num : " + str(self.fail_num) + "  timeout num : " + str(self.timeout_num) + " ================\n")
            f.write("\n")
            if self.fail_num != 0:
                f.write("failed case list:\n")
                for index in self.fail_cases_list:
                    f.write(index + "\n")
            f.write("\n")
            if self.timeout_num != 0:
                f.write("timeout case list:\n")
                for index in self.timeout_cases_list:
                    f.write(index + "\n")
            f.close()
