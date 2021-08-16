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
this script is used for modify the test.cfg of JAVA test cases, note the spaces!
1. ,EXTRA_JAVA_FILE = "a.java:b.java" ---> EXTRA_JAVA_FILE = [a.java,b.java]
2. ,EXTRA_JAVA_FILE = "a.java" ---> EXTRA_JAVA_FILE = a.java
3. ,EXTRA_JAVA_FILE = 'a.java' ---> EXTRA_JAVA_FILE = a.java
4. ,EXTRA_JAVA_FILE = 'a.java:b.java' ---> EXTRA_JAVA_FILE = [a.java,b.java]
5. ,EXTRA_JAVA_FILE = 'a.java,' ---> EXTRA_JAVA_FILE = a.java
'''

import os, re
cur_path = os.path.abspath(__file__)
HOME_PATH = os.path.dirname(os.path.dirname(os.path.dirname(cur_path)))     # ouroboros project

# get test.cfg in all test cases
def record_all_testCFG(PATH):
    compile_record = open("compile_record.txt",'w')     # save compile command
    run_record = open("run_record.txt",'w')             # save run command
    for fpathe,dirs,fs in os.walk(PATH):
        for file in fs:
            if file == 'test.cfg':
                f_tmp = open(os.path.join(fpathe, file),'r')
                try:
                    line = f_tmp.readline()
                    while line != "":
                        if 'compile(' in line:
                            compile_record.write(line.strip() + " @" + fpathe + '\n')
                        elif 'run(' in line:
                            run_record.write(line.strip() + " @" + fpathe + '\n')
                        line = f_tmp.readline()
                    f_tmp.close()
                except:
                    print(os.path.join(fpathe, file)+"  open fail")
    compile_record.close()
    run_record.close()

def quote2bracket_ExtraJavaFile(command, log=False):
    if type(command) != str:
        return
    new_command = command.replace(" ", "")
    new_command = re.sub(r':(?=.+(\"|\'))', ',', new_command)                   # : -> ,
    new_command = re.sub(r'(?<=,EXTRA_JAVA_FILE=)(\"|\')', '[', new_command)    # '/" -> [
    new_command = re.sub(r'(\"|\')(?=\)|,)', ']', new_command)                  # '/" -> ]
    if log and (command != new_command):
        print(command + " ---> " + new_command)
    return new_command

def modify_testCFG(file):
    new_commands = []
    f = open(file,'r+')
    lines = f.readlines()
    f.close()
    for line in lines:
        if 'compile(' in line and "EXTRA_JAVA_FILE" in line:
            new_commands.append(quote2bracket_ExtraJavaFile(line,True))
        else:
            new_commands.append(line)
    f = open(file,'w+')
    f.writelines(new_commands)
    f.close()

def main():
    # record_all_testCFG(HOME_PATH)
    for fpathe,dirs,fs in os.walk(HOME_PATH):
        for file in fs:
            if file == 'test.cfg':
                try:
                    modify_testCFG(os.path.join(fpathe, file))
                except:
                    print(os.path.join(fpathe, file)+"  open fail")

if __name__ == '__main__':
    main()
