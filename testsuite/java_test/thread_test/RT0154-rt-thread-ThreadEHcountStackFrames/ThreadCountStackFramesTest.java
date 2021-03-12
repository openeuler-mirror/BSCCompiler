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


public class ThreadCountStackFramesTest extends Thread {
    static int cnt = 0;
    static Object ob = "aa";
    public static void main(String[] args) {
        ThreadCountStackFramesTest thCountStackFramesTest = new ThreadCountStackFramesTest();
        synchronized (ob) {
            thCountStackFramesTest.start();
            try {
                ob.wait();
            } catch (InterruptedException e) {
                System.out.println("Main wait is interrupted");
            }
        }
        try {
            cnt = thCountStackFramesTest.countStackFrames();
        } catch (IllegalThreadStateException e) {
            System.out.println(2);
            return;
        }
        int j = thCountStackFramesTest.getStackTrace().length;
        if (j == cnt) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        synchronized (ob) {
            ob.notify();
            try {
                ob.wait(1000);
            } catch (InterruptedException e) {
                System.out.println("Thread wait is interrupted");
            }
        }
    }
}
