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


public class ThreadTest {
    public static void main(String[] args) throws Exception {
        //for  join(long millis)
        ThreadRunning threadRunning = new ThreadRunning();
        threadRunning.start();
        try {
            threadRunning.join(-1);
            System.out.println("join(millis) OK");
            threadRunning.stopWork = true;
        } catch (IllegalArgumentException e) {
            threadRunning.stopWork = true;
            System.out.println("in join(millis), millis is negative");
        }
        ThreadRunning threadRunning2 = new ThreadRunning();
        threadRunning2.start();
        try {
            threadRunning2.join(1000);
            System.out.println("join(millis) OK");
            threadRunning2.stopWork = true;
        } catch (IllegalArgumentException e) {
            threadRunning2.stopWork = true;
            System.out.println("in join(millis), millis is negative");
        }
        //for join(long millis, int nanos)
        ThreadRunning threadRunning3 = new ThreadRunning();
        threadRunning3.start();
        try {
            threadRunning3.join(1000, 1123456);
            System.out.println("join(millis, nanos) OK");
            threadRunning3.stopWork = true;
        } catch (IllegalArgumentException e) {
            threadRunning3.stopWork = true;
            System.out.println("in join(millis, nanos), nanosecond out of range");
        }
        ThreadRunning threadRunning4 = new ThreadRunning();
        threadRunning4.start();
        try {
            threadRunning4.join(-2, 123456);
            System.out.println("join(millis, nanos) OK");
            threadRunning4.stopWork = true;
        } catch (IllegalArgumentException e) {
            threadRunning4.stopWork = true;
            System.out.println("in join(millis, nanos), millis is negative");
        }
        ThreadRunning threadRunning5 = new ThreadRunning();
        threadRunning5.start();
        try {
            threadRunning5.join(1000, 123456);
            System.out.println("join(millis, nanos) OK");
            threadRunning5.stopWork = true;
        } catch (IllegalArgumentException e) {
            threadRunning5.stopWork = true;
            System.out.println("in join(millis, nanos), millis is negative or nanosecond out of range");
        }
        System.out.println("PASS");
    }
    static class ThreadRunning extends Thread {
        public volatile int i = 0;
        volatile boolean stopWork = false;
        ThreadRunning() {
            super();
        }
        public void run() {
            while (!stopWork) {
                i++;
                i--;
            }
        }
    }
}