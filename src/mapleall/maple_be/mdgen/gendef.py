#!/usr/bin/env python
# coding=utf-8
#
# Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#
import os, sys, subprocess, shlex, re, argparse
def Gendef(execTool, mdFiles, outputDir, asanLib=None):
  tdList = []
  for mdFile in mdFiles:
    if mdFile.find('sched') >= 0:
      schedInfo = mdFile
      mdCmd = "%s --genSchdInfo %s -o %s" %(execTool, schedInfo , outputDir)
      isMatch = re.search(r'[;\\|\\&\\$\\>\\<`]', mdCmd, re.M|re.I)
      if (isMatch):
        print("Command Injection !")
        return
      print("[*] %s" % (mdCmd))
      localEnv = os.environ
      if asanLib is not None:
        asanEnv = asanLib.split("=")
        localEnv[asanEnv[0]] = asanEnv[1]
        print("env :{}".format(str(asanEnv)))
      subprocess.check_call(shlex.split(mdCmd), shell = False, env = localEnv)
    else:
      tdList.append(i)
  return

def Process(execTool, mdFileDir, outputDir, asanLib=None):
  if not (os.path.exists(execTool)):
    print("maplegen is required before generating def files automatically")
    return
  if not (os.path.exists(mdFileDir)):
    print("td/md files is required as input!!!")
    print("Generate def files FAILED!!!")
    return

  mdFiles = []
  for root,dirs,allfiles in os.walk(mdFileDir):
    for mdFile in allfiles:
      mdFiles.append("%s/%s"%(mdFileDir, mdFile))

  if not (os.path.exists(outputDir)):
    print("Create the " + outputDir)
    os.makedirs(outputDir)
    Gendef(execTool, mdFiles, outputDir, asanLib)

  defFile = "%s/mplad_arch_define.def" % (outputDir)
  if not (os.path.exists(defFile)):
    Gendef(execTool, mdFiles, outputDir, asanLib)
  for mdfile in mdFiles:
    if (os.stat(mdfile).st_mtime > os.stat(defFile).st_mtime):
      Gendef(execTool, mdFiles, outputDir, asanLib)
  if (os.stat(execTool).st_mtime > os.stat(defFile).st_mtime):
    Gendef(execTool, mdFiles, outputDir, asanLib)

def get_arg_parser():
  parser = argparse.ArgumentParser(
      description="maplegen")
  parser.add_argument('-e', '--exe',
                      help='maplegen_exe_directory')
  parser.add_argument('-m', '--md',
                      help='mdfiles_directory')
  parser.add_argument('-o', '--out',
                      help='output_defiless_directory')
  parser.add_argument('-a', '--asan',
                      help='enabled asan and followed env LD_PRELOAD=xxxx')
  return parser

def main():
  parser = get_arg_parser()
  args = parser.parse_args()
  if (args.exe is None or args.md is None or args.out is None):
    print(str(args))
    parser.print_help()
    exit(-1)

  Process(args.exe, args.md, args.out, args.asan)

if __name__ == "__main__":
  main()
