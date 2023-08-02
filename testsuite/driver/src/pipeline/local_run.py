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
import time

import target_parser
import result_parser
from case import case_pipeline
from multiprocessing import Pool, cpu_count


class LocalRun(object):

    def __init__(self, input):
        self.input = input

    def local_run_pipeline(self):
        results = {}
        retry = self.input["retry"]
        target_parse = target_parser.common_parse.CommonParse(self.input)
        target_parse.execute()
        cases = target_parse.get_output()
        pool = Pool(min(cpu_count(), self.input["jobs"]))
        return_results = []
        for case_name in cases.keys():
            self.input["case_name"] = case_name
            self.input["mode_set"] = cases[case_name]
            case_run = case_pipeline.origin_run.OriginRun(self.input.copy())
            return_results.append(pool.apply_async(case_run.execute, ()))
        finished = 0
        total = len(cases)
        while total != finished:
            time.sleep(1)
            finished = sum([return_result.ready() for return_result in return_results])
        for return_result in return_results:
            results.update(return_result.get())
        pool.close()
        pool.join()
        result_parse = result_parser.origin_parse.OriginParse(results)
        result_parse.result_parse()
        result_parse.gen_report()
        result_parse.print_report()
        while not result_parse.all_pass() and retry > 1 and result_parse.all_failed_num < 20:
            print("================ Test Again (residual retry:%d)================="%(retry - 1))
            again_suite = result_parse.get_again_suite()
            if not again_suite:
                sys.exit(1)
            print(again_suite)
            results = {}
            for case_name in again_suite.keys():
                self.input["case_name"] = case_name
                self.input["mode_set"] = again_suite[case_name]
                self.input["detail"] = True
                case_run = case_pipeline.origin_run.OriginRun(self.input.copy())
                results.update(case_run.execute())
            result_parse = result_parser.origin_parse.OriginParse(results)
            result_parse.result_parse()
            result_parse.gen_report()
            result_parse.print_report()
            retry -= 1
        result_parse.write_report()
        if not result_parse.all_pass():
            sys.exit(1)
