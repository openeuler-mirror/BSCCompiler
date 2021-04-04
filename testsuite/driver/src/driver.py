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
import os
import sys
import shutil
import optparse
import multiprocessing

from env_var import EnvVar
from target import Target
from mode_table import ModeTable
from case import Case
from task import RunCaseTask, RunCasesTask, CleanCaseTask, GenShellScriptTask

init_optparse = optparse.OptionParser()
env_args = init_optparse.add_option_group("Environment options")
env_args.add_option('--sdk-root', dest="sdk_root", default=None, help='Sdk location')
env_args.add_option('--test-suite-root', dest="test_suite_root", default=None, help='Test suite location')
env_args.add_option('--source-code-root', dest="source_code_root", default=None, help='Source code location')

target_args = init_optparse.add_option_group("Target options")
target_args.add_option('--target', dest="target", help='Target. It can be a case, a module or a target suite')
env_args.add_option('--cmd-json-output', dest="cmd_json_output", default=None, help='Cmd json output location')
env_args.add_option('--report', dest="report", default=None, help="Report output location")
# TODO: Add executor
target_args.add_option('-j', '--jobs', dest="jobs", type=int, default=multiprocessing.cpu_count(), help='Number of parallel jobs')
target_args.add_option('--retry', dest="retry", action='store_true', default=False, help='Rerun failed and timeout cases')
target_args.add_option('--run-path', dest="run_path", default=None, help='Where to run cases')
target_args.add_option('--mode', dest="mode", default=None, help='Which mode to run')
# TODO: Add phone test
target_args.add_option('--platform', dest="platform", default="qemu", help='qemu or phone')
target_args.add_option('--clean', dest="clean", action='store_true', default=False, help='clean the path')

package = {
    'container_image': "cpl-eqm-docker-sh.rnd.huawei.com/valoran-run:0.6",
    'case_retry_num': 4,
    'total_retry_num': 1000
}


# TODO: Only Copy dirs to be used
def prebuild(run_path):
    if run_path is not None:
        if os.path.exists(run_path):
            shutil.rmtree(run_path)
        os.makedirs(run_path)
        for dir in os.listdir(EnvVar.TEST_SUITE_ROOT):
            if dir.endswith("_test"):
                shutil.copytree(os.path.join(EnvVar.TEST_SUITE_ROOT, dir), os.path.join(run_path, dir))
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
    target = opt.target
    clean = opt.clean
    cmd_json_output = opt.cmd_json_output
    jobs = opt.jobs
    retry = opt.retry
    mode = opt.mode
    report = opt.report

    EnvVar(sdk_root, test_suite_root, source_code_root)
    prebuild(run_path)

    mode_tables = {}
    if target + ".conf" in os.listdir(EnvVar.CONFIG_FILE_PATH):
        mode_tables[target] = modeTable(os.path.join(EnvVar.CONFIG_FILE_PATH, target + ".conf"))
    else:
        for file in os.listdir(EnvVar.CONFIG_FILE_PATH):
            if file.endswith("conf") and file != "testallops.conf" :
                mode_tables[file.split(".")[0]] = modeTable(os.path.join(EnvVar.CONFIG_FILE_PATH, file))

    if clean:
        target_mode_set = set()
        for mode_table in mode_tables.keys():
            target_mode_set = target_mode_set.union(mode_tables[mode_table].get_target_mode_set(target))
        the_case = Case(target, target_mode_set)
        task = CleanCaseTask(the_case)
        task.run()
        sys.exit(0)

    if target in mode_tables.keys():
        targets = mode_tables[target].get_targets()
    elif os.path.exists(os.path.join(EnvVar.TEST_SUITE_ROOT, target)):
        targets = [target]
    else:
        print("Target " + target + " doesn't exist !")
        sys.exit(1)

    case_names = set()
    cases = []
    for target in targets:
        the_target = Target(target)
        case_names = case_names.union(the_target.get_cases())

    for case_name in case_names:
        mode_set = set()
        default_mode_set = set()
        for mode_table in mode_tables.keys():
            default_mode_set = default_mode_set.union(mode_tables[mode_table].get_target_mode_set(case_name))
        if mode is not None:
            if mode in default_mode_set:
                mode_set = {mode}
            elif len(case_names) == 1:
                print(case_name + " doesn't have OPT " + mode)
                print(case_name + " OPT : " + str(default_mode_set))
                sys.exit(1)
        else:
            mode_set = default_mode_set
        if len(mode_set) != 0:
            cases.append(Case(case_name, mode_set))

    if cmd_json_output is not None:
        task = GenShellScriptTask(cases)
        task.run()
        package["tasks"] = task.get_command_package()
        with open(opt.cmd_json_output, 'w') as f:
            json.dump(package, f)
    elif len(cases) == 1:
        task = RunCaseTask(cases[0], detail=True)
        task.run()
    else:
        task = RunCasesTask(cases, process_num=jobs, detail=False)
        task.run()
        task.result_parse()
        if retry:
            task.run_again()
            if report is not None:
                task.gen_report(report)
            sys.exit(1)

if __name__ == '__main__':
    main(sys.argv[1:])
