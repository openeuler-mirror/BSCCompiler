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

from env_var import EnvVar


class OriginParse(object):

    def __init__(self, results: dict):
        self.results = results
        self.report = []
        self.all_passed_num = 0
        self.all_failed_num = 0
        self.all_timeout_num = 0
        self.failed_suite = {}
        self.timeout_suite = {}

    def result_parse(self):
        for case_name in self.results.keys():
            passed_num = len(self.results[case_name]["PASSED"])
            failed_num = len(self.results[case_name]["FAILED"])
            timeout_num = len(self.results[case_name]["TIMEOUT"])
            self.all_passed_num += passed_num
            self.all_failed_num += failed_num
            self.all_timeout_num += timeout_num
            if failed_num != 0:
                self.failed_suite[case_name] = self.results[case_name]["FAILED"]
            if timeout_num != 0:
                self.timeout_suite[case_name] = self.results[case_name]["TIMEOUT"]

    def gen_report(self):
        if self.all_failed_num == 0 and self.all_timeout_num == 0:
            self.report.append("============ ALL CASE RUN SUCCESSFULLY ================")
            return
        self.report.append("=========== case total num : " + str(
            self.all_passed_num + self.all_failed_num + self.all_timeout_num) + "  passed num : " + str(
            self.all_passed_num) + "  failed num : " + str(self.all_failed_num) + "  timeout num : " + str(
            self.all_timeout_num) + " ================\n\n")
        if self.all_failed_num != 0:
            self.report.append("failed case list:\n")
            for case_name in self.failed_suite:
                self.report.append(case_name + ": " + ",".join(self.failed_suite[case_name]))
        if self.all_timeout_num != 0:
            self.report.append("timeout case list:\n")
            for case_name in self.timeout_suite:
                self.report.append(case_name + ": " + ",".join(self.timeout_suite[case_name]))

    def print_report(self):
        for line in self.report:
            print(line)

    def write_report(self):
        with open(EnvVar.SOURCE_CODE_ROOT + "/report.txt", "wt") as f:
            for line in self.report:
                f.write(line)

    def all_pass(self):
        return self.all_failed_num == 0 and self.all_timeout_num == 0

    def get_again_suite(self):
        again_suite = self.failed_suite
        for case_name in self.timeout_suite.keys():
            if case_name in again_suite.keys():
                again_suite[case_name] = again_suite[case_name] | self.timeout_suite[case_name]
            else:
                again_suite[case_name] = self.timeout_suite[case_name]
        return again_suite

