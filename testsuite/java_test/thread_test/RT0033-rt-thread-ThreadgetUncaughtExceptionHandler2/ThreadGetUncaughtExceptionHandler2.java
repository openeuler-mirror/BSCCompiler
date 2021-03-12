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


public class ThreadGetUncaughtExceptionHandler2 extends Thread{
    static String string;
    public void run() {
        System.out.println(3/0);
    }
    public static void main(String[] args) {
        ThreadGetUncaughtExceptionHandler2 threadGetUncaughtExceptionHandler2 = new
                ThreadGetUncaughtExceptionHandler2();
        Thread.setDefaultUncaughtExceptionHandler(new UncaughtExceptionHandler() {
            public void uncaughtException(Thread t, Throwable e) {
            }
        });
        threadGetUncaughtExceptionHandler2.start();
        string = threadGetUncaughtExceptionHandler2.getUncaughtExceptionHandler().toString();
        if(string.indexOf("java.lang.ThreadGroup") != -1) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}
