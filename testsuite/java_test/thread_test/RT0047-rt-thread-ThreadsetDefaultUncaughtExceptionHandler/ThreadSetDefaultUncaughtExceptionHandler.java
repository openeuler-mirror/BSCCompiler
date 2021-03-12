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


import java.util.concurrent.atomic.AtomicInteger;
public class ThreadSetDefaultUncaughtExceptionHandler extends Thread{
    static AtomicInteger i = new AtomicInteger(0);
    static String p;
    public void run() {
        System.out.println(3/0);
    }
    public static void main(String[] args) {
        ThreadSetDefaultUncaughtExceptionHandler thread_obj1 = new ThreadSetDefaultUncaughtExceptionHandler();
        ThreadSetDefaultUncaughtExceptionHandler thread_obj2 = new ThreadSetDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(new UncaughtExceptionHandler() {
            public void uncaughtException(Thread t, Throwable e) {
                p = e.toString();
                if (p.indexOf("java.lang.ArithmeticException") != -1) {
                    i.incrementAndGet();
                }
            }
        });
        thread_obj1.start();
        thread_obj2.start();
        try{
        thread_obj1.join();
        thread_obj2.join();
        }
        catch (InterruptedException e1) {
        }
        if(i.get() == 2) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
}
