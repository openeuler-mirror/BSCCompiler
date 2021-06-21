#!/usr/bin/python3
import sys
import os
import re

funclist=[]
funcMap={}
buffer=''

class AsmFunc:
    funcName = ''
    start = 0
    end = 0
    funcId = 0
    totalLine = 0
    realLine = 0
    blankLine = 0
    varRegLine = 0
    aliasLine = 0
    commentLine = 0
    funcInfo = 7
    locLine = 0
    otherLine = 0
    source = ''
    locList =[]

    def __str__(self):
        return '\n'.join(['%s:%s' % item for item in self.__dict__.items()])
    def calc(self):
        self.totalLine = self.end - self.start - 1
        self.realLine = self.totalLine - 1 - self.funcInfo - self.blankLine - self.varRegLine - self.aliasLine - self.locLine - self.commentLine


def binarySearch (arr, l, r, x):
    while l <= r:
        mid = int(l + (r - l)/2)
        # 元素整好的中间位置
        if arr[mid] == x:
            return mid
        # 元素小于中间位置的元素，只需要再比较左边的元素
        elif arr[mid] > x:
            r = mid-1
        # 元素大于中间位置的元素，只需要再比较右边的元素
        else:
            l = mid+1
    # 不存在
    return -1

def readFunclist(file):
    global funclist
    with open(file, "r") as f:
        for func in f:
            funclist.append(func.strip())
    funclist.sort()

def isMatch(name):
    global funclist
    index = binarySearch(funclist, 0, len(funclist) - 1, name)
    if index != -1:
        return True
    return False

def exportToFile(list, file):
    f = open(file, "a+")
    i = 0
    while i < len(list):
        f.write(list[i].source)
        i += 1
    f.close()

def processAsm(mpl):
    global funcMap
    with open(mpl, "r") as f:
        infunc = False
        match = False
        flag = False
        curfunc = AsmFunc()
        for ori_line in f:
            line = ori_line.strip()
            if not match:
                if line.endswith("%function"):
                    funcName = line.split()[1][:-1]
                    match=isMatch(funcName)
                    if match:
                        func = AsmFunc()
                        func.funcName = funcName
                        funcMap[funcName] = func
                        curfunc = func
                        continue
            else:
                if not infunc:
                    if line == curfunc.funcName + ":":
                        infunc = True
                        curfunc.source += ori_line
                else:
                    curfunc.source += ori_line
                    if line.endswith(curfunc.funcName + ", .-" + curfunc.funcName):
                        infunc = False
                        match = False

def repaceGood(mpl):
    global funcMap
    with open(mpl, "r") as f:
        infunc = False
        match = False
        curfunc = AsmFunc()
        for ori_line in f:
            line = ori_line.strip()
            if not match:
                print(ori_line, end='')
                if line.endswith("%function"):
                    funcName = line.split()[1][:-1]
                    match=isMatch(funcName)
                    if match:
                        func = AsmFunc()
                        func.funcName = funcName
                        curfunc = func
                        continue
            else:
                if not infunc:
                    if line == curfunc.funcName + ":":
                        print(funcMap[curfunc.funcName].source, end='')
                        infunc = True
                    else:
                        print(ori_line, end='')
                else:
                    if line.endswith(curfunc.funcName + ", .-" + curfunc.funcName):
                        infunc = False
                        match = False

def main():
    if len(sys.argv) < 4 :
        print("args count less than 4")
        return
    global file1
    good = sys.argv[1]
    bad = sys.argv[2]
    readFunclist(sys.argv[3])
    if os.path.isfile(bad):
        processAsm(bad)
    else:
        print("file not found")
    if os.path.isfile(good):
        repaceGood(good)
    else:
        print("file not found")


if __name__ == "__main__":
    main()

