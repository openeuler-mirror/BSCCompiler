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


public class ThreadGetDefaultUncaughtExceptionHandler1 extends Thread{
    static String p;
    public void run() {
        System.out.println(3/0);
    }
    public static void main(String[] args) {
        ThreadGetDefaultUncaughtExceptionHandler1 cls=new ThreadGetDefaultUncaughtExceptionHandler1();
        Thread.setDefaultUncaughtExceptionHandler(new UncaughtExceptionHandler() {
            public void uncaughtException(Thread t, Throwable e) {
            }
        });
        cls.start();
        try{
            cls.join();
        }
        catch (InterruptedException e1) {
        }
        p=Thread.getDefaultUncaughtExceptionHandler().toString();
        if(p.indexOf("ThreadGetDefaultUncaughtExceptionHandle") !=-1) {
            if(p.toString().indexOf("$") !=-1) {
                if(p.toString().indexOf("@") !=-1) {
                    System.out.println(0);
                }
            }
        }
    }
}
