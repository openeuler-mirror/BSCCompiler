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
    // Enumerate() does not work in maple.
    public static void main(String[] args) throws Exception {
        ThreadRunning t1 = new ThreadRunning();
        t1.start();
        ThreadRunning t2 = new ThreadRunning();
        t2.start();
        ThreadRunning t11 = new ThreadRunning();
        t11.start();
        ThreadRunning t12 = new ThreadRunning();
        t12.start();
        ThreadRunning t121 = new ThreadRunning();
        t121.start();
        ThreadRunning t122 = new ThreadRunning();
        t122.start();
        // Estimate dimension as 6 created threads.
        // Plus 10 for some other threads.
        int estimateLength = 16;
        Thread[] list;
        int count;
        while (true) {
            list = new Thread[estimateLength];
            count = Thread.enumerate(list);
            if (count == estimateLength) {
                estimateLength *= 2;
            } else {
                //System.out.println(count);
                break;
            }
        }
        t1.stopWork = true;
        t2.stopWork = true;
        t11.stopWork = true;
        t12.stopWork = true;
        t121.stopWork = true;
        t122.stopWork = true;
        System.out.println("thread count : " + count);
    }
    // Test for enumerate()
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
