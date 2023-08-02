#!/usr/bin/env python
# coding=utf-8

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
import optparse
import subprocess
from env_var import EnvVar
from basic_tools.file import get_test_and_nontest_curdir
from pipeline.case_clean import CaseClean
from pipeline.cicd_run import CICDRun
from pipeline.local_run import LocalRun
from pipeline.save_tmp import SaveTmp

init_optparse = optparse.OptionParser()
env_args = init_optparse.add_option_group("Environment options")
env_args.add_option('--sdk-root', dest="sdk_root", default=None, help='Sdk location')
env_args.add_option('--test-suite-root', dest="test_suite_root", default=None, help='Test suite location')
env_args.add_option('--source-code-root', dest="source_code_root", default=None, help='Source code location')

target_args = init_optparse.add_option_group("Target options")
target_args.add_option('--target', dest="target", help='Target. It can be a case, a module or a target suite')
# TODO: Add executor
target_args.add_option('-j', '--jobs', dest="jobs", type=int, default=1, help='Number of parallel jobs')
target_args.add_option('--run-path', dest="run_path", default=None, help='Where to run cases')
target_args.add_option('--mode', dest="mode", default=None, help='Which mode to run')
target_args.add_option('--detail', dest="detail", action="store_true", default=False, help="show details")
# TODO: Add phone test
target_args.add_option('--platform', dest="platform", default="qemu", help='qemu or phone')
target_args.add_option('--clean', dest="clean", action='store_true', default=False, help='clean the path')
target_args.add_option('--save', dest="save", action='store_true', default=False, help='save temp files')
target_args.add_option('--cicd', dest="cicd", action='store_true', default=False, help='cicd use')
target_args.add_option('--timeout', dest="TIMEOUT", type=int, default=300, help='control all cases timeout')
target_args.add_option('--retry', dest="RETRY", type=int, default=2, help='control all cases retry')


# TODO: Only Copy dirs to be used
def prebuild(run_path, target):
    if run_path is None:
        return
    test_suites, _ = get_test_and_nontest_curdir(EnvVar.TEST_SUITE_ROOT)
    if target.startswith("testall"):
        target_suites = test_suites
    elif "_test" in target:
        target_suites = [target]
        test_suite = target.split('/')[0]
        _, nontest_suite = get_test_and_nontest_curdir(os.path.join(EnvVar.TEST_SUITE_ROOT, test_suite))
        # cp c_test/lib
        for sub_nontest in nontest_suite:
            orig_sub_nontest = os.path.join(EnvVar.TEST_SUITE_ROOT, test_suite, sub_nontest)
            dest_sub_nontest = os.path.join(run_path, test_suite, sub_nontest)
            subprocess.run("mkdir -p {}".format(dest_sub_nontest), shell=True, check=True)
            subprocess.run("rsync -r {}/ {}/".format(orig_sub_nontest, dest_sub_nontest), shell=True, check=True)
    for testsuite in target_suites:
        orig_target = os.path.join(EnvVar.TEST_SUITE_ROOT, testsuite)
        dest_target = os.path.join(run_path, testsuite)
        subprocess.run("mkdir -p {}".format(dest_target), shell=True, check=True)
        subprocess.run("rsync -r {}/ {}/ --delete".format(orig_target, dest_target), shell=True, check=True)
    EnvVar.TEST_SUITE_ROOT = run_path


def main(orig_args):
    opt, args = init_optparse.parse_args(orig_args)
    if args:
        init_optparse.print_usage()
        sys.exit(1)

    sdk_root = opt.sdk_root
    test_suite_root = opt.test_suite_root
    source_code_root = opt.source_code_root
    run_path = opt.run_path
    input = {
        "target": opt.target,
        "jobs": opt.jobs,
        "mode": opt.mode,
        "detail": opt.detail,
        "timeout": opt.TIMEOUT,
        "retry": opt.RETRY
    }
    clean = opt.clean
    save = opt.save
    cicd = opt.cicd

    EnvVar(sdk_root, test_suite_root, source_code_root)
    prebuild(run_path, input['target'])
    if clean:
        task = CaseClean(input)
        task.case_clean_pipeline()
    elif save:
        task = SaveTmp(input)
        task.save_tmp_pipeline()
    elif cicd:
        task = CICDRun(input)
        task.cicd_run_pipeline()
    else:
        task = LocalRun(input)
        task.local_run_pipeline()


if __name__ == '__main__':
    main(sys.argv[1:])
