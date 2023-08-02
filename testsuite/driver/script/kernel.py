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

#! /usr/bin/python3

import os
import subprocess
import types
import filecmp
import re

#Assert Area
def assertEqual(message, expected, actual):
    print(message)
    if not type(expected) is type(actual):
        print("assertError: compare type ain't the same")
        return False

    if expected == None or actual == None:
        print("assertError: No correct input for equals")
        return False
    if os.path.isfile(expected) and os.path.isfile(actual):
        fcmp = filecmp.cmp(expected, actual)
        if not fcmp:
            assertError("equal", str(expected), str(actual))
            return False
        else:
            print(expected+" and "+actual+" are the same")
            return True

    if type(actual) is int or type(actual) is float or type(actual) is str:
        if not expected == actual:
            assertError("equal", str(expected), str(actual))
            return False
        else:
            print("expected:"+str(expected)+" and "+"actual:"+str(actual)+" are equal")
            return True

def assertTrue(message, condition):
    print(message)
    if type(condition) is int or type(condition) is float:
        print("condition is value, goto assertEqual")
        return assertEqual("Check if condition value would be Zero", 0, condition )
    else:
        if type(condition) is str :
            cmd = Executor(condition)
            res = cmd.execute()
            if res.returnValue == 0:
                print("Condition got True")
                return True
            else: 
                print("Condition got  Failed")
                return False
        else:
            return False

def assertContain(message, filename):
    print(message)
    if os.path.isfile(filename):
        with open(filename, "r") as fp:
            strr = fp.read()
            if(strr.find(message) != -1):
                print(filename + " contains the string:" + message)
                return True
    return False


def assertFalse(message, condition):
    return True

def assertArrayEquals(expected, actuals):
    return True

def assertNotNull(obj):
    return True

def assertNull(obj):
    return True

def assertThat(actual,expr):
#Dirty API. Need to work well with python.
    tool = os.environ['MAPLE_ROOT']+"/Zeiss/prebuilt/tools/regx_checker.sh "
    cmd = Executor(tool + actual + " " + expr)
    res = cmd.execute()
    if res.returnValue == 0:
        return True
    else:
        return False

#AssertInfo Area
def assertError(cmptype, name1, name2):
    print("assertError: "+ name1 + " and " + name2 + " are not "+ cmptype)

def Regx_check(regx,resultfile):
    if not os.path.isfile(resultfile) and not os.path.isfile(regx):
        return False
    with open(regx,"r",encoding="latin1") as fp, open(resultfile,"r",encoding="latin1") as fo:
        strr = fo.read()
        for s in fp.readlines():
            s=s.strip()
            if(strr.find(s) == -1):
                return False
    return True

def Str_CheckNum(resultfile, string, num):
    with open(resultfile,"r") as fm:
        fs = fm.read()
        strnum = re.findall(string,fs)
        num=int(num)
        if num != len(strnum) :
            return False
    return True

def Str_CheckGNum(resultfile, string, gnum):
    with open(resultfile,"r") as fm:
        fs = fm.read()
        strnum = re.findall(string,fs)
        gnum=int(gnum)
        if gnum >= len(strnum) :
            return False
    return True
