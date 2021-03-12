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


public class ThreadSetUncaughtExceptionHandler extends Thread {
    static int i = 0;
    static String p;
    public void run() {
        System.out.println(3/0);
    }
    public static void main(String[] args) {
        ThreadSetUncaughtExceptionHandler cls = new ThreadSetUncaughtExceptionHandler();
        ThreadSetUncaughtExceptionHandler cls2 = new ThreadSetUncaughtExceptionHandler();
        cls.setUncaughtExceptionHandler(new UncaughtExceptionHandler() {
            public void uncaughtException(Thread t, Throwable e) {
                p = e.toString();
                if (p.indexOf("java.lang.ArithmeticException") != -1) {
                    i++;
                }
            }
        });
        cls2.setUncaughtExceptionHandler(new UncaughtExceptionHandler() {
            public void uncaughtException(Thread t, Throwable e) {
            }
        });
        cls.start();
        cls2.start();
        try {
            cls.join();
            cls2.join();
        } catch (InterruptedException e1) {
            System.out.println("Join is interrupted");
        }
        if(i == 1) {
            System.out.println(0);
        }
    }
}
