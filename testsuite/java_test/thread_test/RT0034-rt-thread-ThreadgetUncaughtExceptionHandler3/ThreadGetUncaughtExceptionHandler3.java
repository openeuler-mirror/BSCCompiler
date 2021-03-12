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


public class ThreadGetUncaughtExceptionHandler3 extends Thread{
    static String string;
    public void run() {
        System.out.println(3/0);
    }
    public static void main(String[] args) {
        ThreadGetUncaughtExceptionHandler3 threadGetUncaughtExceptionHandler3 = new
                ThreadGetUncaughtExceptionHandler3();
        threadGetUncaughtExceptionHandler3.setUncaughtExceptionHandler(new UncaughtExceptionHandler() {
            public void uncaughtException(Thread t, Throwable e) {
            }
        });
        threadGetUncaughtExceptionHandler3.start();
        try{
            threadGetUncaughtExceptionHandler3.join();
        } catch (InterruptedException e1) {
            System.out.println("Join is interrupted");
        }
        try {
            string = threadGetUncaughtExceptionHandler3.getUncaughtExceptionHandler().toString();
            if (string.indexOf("ThreadGetUncaughtExceptionHandler3") != -1) {
                if (string.toString().indexOf("$") != -1) {
                    if (string.toString().indexOf("@") != -1) {
                        System.out.println(0);
                        return;
                    }
                }
            }
        } catch (NullPointerException e2) {
            System.out.println(2);
        }
    }
}
