/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/


import java.io.PrintStream;
public class ThreadgetStackTrace extends Thread {
    public static void main(String[] args) {
        System.out.println(run(args, System.out));
    }
    public static int run(String[] args, PrintStream out) {
        StackTraceElement[] stackTraceElements = Thread.currentThread().getStackTrace();
        int l = stackTraceElements.length;
        if (stackTraceElements[l - 1].toString().contains("main") && stackTraceElements[l - 2].toString().contains("run") && stackTraceElements[l - 3].toString().contains("getStackTrace")) {
            return 0;
        }
        return 2;
    }
}
