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
import kernel
import paramiko
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-c", "--check", dest="check", default="assert", help="This is the kind of log check.Three kinds can be choose: assert regx")
parser.add_option("-r", "--regx", dest="regx", default="regx", help="This is the kind of log check.")
parser.add_option("-f", "--result", dest="result", default="output.log", help="This is the filename used to compare")
parser.add_option("-s", "--str", dest="str", default="", help="This is the string used to compare")
parser.add_option("-e", "--expected", dest="expected", default="expected.txt", help="This is the filename used to compare")
parser.add_option("-n", "--num", dest="num", default="0", help="This is the filename used to compare")
parser.add_option("-g", "--gnum", dest="gnum", default="0", help="This is the filename used to compare, file counts more than gnum is OK")


(options, args) = parser.parse_args()

if options.check == "assert":
    checkstatus = kernel.assertEqual("", options.expected, options.result)
    if checkstatus :
        print("\033[32;1m assert check pass \033[0m")
    else:
        print("\033[31;1m FAIL : " + options.expected + " and " + options.result + " are not equal \033[0m")

if options.check == "regx":
    checkstatus = kernel.Regx_check(options.regx, options.result)
    if checkstatus :
        print("\033[32;1m regx check pass \033[0m")
    else:
        print("\033[31;1m FAIL : regx cannot be found in " + options.result  + " \033[0m")

if options.check == "contain":
    checkstatus = kernel.assertContain(options.str, options.result)
    if checkstatus :
        print("\033[32;1m contain check pass \033[0m")
    else:
        print("\033[31;1m FAIL : string cannot be found in " + options.result  + "  \033[0m")

if options.check == "num":
    checkstatus = kernel.Str_CheckNum(options.result, options.str, options.num)
    if checkstatus :
        print("\033[32;1m  num check pass \033[0m")
    else:
        print("\033[31;1m FAIL : the number of" + options.str + "in " + options.result + " is incorrect  \033[0m")

if options.check == "gnum":
    checkstatus = kernel.Str_CheckGNum(options.result, options.str, options.gnum)
    if checkstatus :
        print("\033[32;1m  num check pass \033[0m")
    else:
        print("\033[31;1m FAIL : the number of" + options.str + "in " + options.result + " is incorrect  \033[0m")

if options.check == "not_contain":
    checkstatus = kernel.assertContain(options.str, options.result)
    checkstatus = not checkstatus
    if checkstatus :
        print("\033[32;1m not contain check pass \033[0m")
    else:
        print("\033[31;1m FAIL : string can be found in " + options.result  + "  \033[0m")

sys.exit(checkstatus == False)
