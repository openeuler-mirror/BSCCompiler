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

'''
this script is used to remove 'clean()' in test.cfg of each test case;
and also remove 'clean' command in mode/*.py
'''

import os
import re
import sys
import optparse

cur_path = os.path.abspath(__file__)
HOME_PATH = os.path.dirname(os.path.dirname(os.path.dirname(cur_path)))     # ouroboros project
MODE_PATH = os.path.join(HOME_PATH, 'driver', 'src', 'mode')
MODE_OPS_PATH = os.path.join(HOME_PATH, 'driver', 'src', 'mode_ops')

init_opt = optparse.OptionParser()
init_opt.add_option('-f', '--file', dest="one_file", default=None, help='it must be test.cfg with abs path')
init_opt.add_option('-a', '--all', dest="all_files", action='store_true',default=False, help='whether to modify all cases test.cfg')
init_opt.add_option('-m', '--mode', dest="mode_path", default=None, help='~/ouroboros/driver/src/mode path')

def remove_clean_in_testCFG(file):
    with open(file,'r+') as f:
        old_commands = f.read()
    new_commands = old_commands.replace('clean()\n','')
    with open(file, 'w+') as f:
        f.writelines(new_commands)

def remove_clean_command_in_mode(path):
    file_pattern = r'[A-Z0-9_]+\.py'
    clean_pattern = r'\    "clean\": \[[\s]*Shell\([\s]*"rm -rf .*"[\s]*\)[\s]*\]\,\n(?=[\s]*")'
    for file in os.listdir(path):
        if re.match(file_pattern, file) is None:
            continue
        with open(os.path.join(path, file), 'r+') as f:
            file_content = f.read()
        new_file_content = re.sub(clean_pattern, '', file_content)
        with open(os.path.join(path, file), 'w+') as f:
            f.write(new_file_content)

def main(orig_args):
    opt, args = init_opt.parse_args(orig_args)
    if args:
        init_optparse.print_usage()
        sys.exit(1)
    file = opt.one_file
    is_all = opt.all_files
    mode_path = opt.mode_path
    if is_all:
        remove_clean_command_in_mode(MODE_PATH)
        remove_clean_command_in_mode(MODE_OPS_PATH)
        for fpathe,dirs,fs in os.walk(os.path.dirname(HOME_PATH)):
            for file in fs:
                if file == 'test.cfg':
                    try:
                        remove_clean_in_testCFG(os.path.join(fpathe, file))
                    except:
                        print(os.path.join(fpathe, file)+"  open fail")
    elif file is not None and file.endswith('test.cfg'):
        remove_clean_in_testCFG(os.path.join(HOME_PATH, file))

    if mode_path is not None:
        remove_clean_command_in_mode(mode_path)

if __name__ == '__main__':
    main(sys.argv[1:])
