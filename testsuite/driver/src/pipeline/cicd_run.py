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

import json
import time

import target_parser
from case import case_pipeline
from multiprocessing import Pool, cpu_count

from env_var import EnvVar


class CICDRun(object):

    def __init__(self, input):
        self.input = input

    def cicd_run_pipeline(self):
        results = []
        target_parse = target_parser.common_parse.CommonParse(self.input)
        target_parse.execute()
        cases = target_parse.get_output()
        pool = Pool(min(cpu_count(), self.input["jobs"]))
        return_results = []
        for case_name in cases.keys():
            self.input["case_name"] = case_name
            self.input["mode_set"] = cases[case_name]
            case_run = case_pipeline.cicd_run.CICDRun(self.input.copy())
            return_results.append(pool.apply_async(case_run.execute, ()))
        finished = 0
        total = len(cases)
        while total != finished:
            time.sleep(1)
            finished = sum([return_result.ready() for return_result in return_results])
        for return_result in return_results:
            results += return_result.get()
        pool.close()
        pool.join()
        package = {
            "container_image": "cpl-eqm-docker-sh.rnd.huawei.com/valoran-run:0.6",
            "case_retry_num": 4,
            "total_retry_num": 1000,
            "tasks": results
        }
        with open(EnvVar.SOURCE_CODE_ROOT + "/json.txt", "wt") as f:
            json.dump(package, f)
