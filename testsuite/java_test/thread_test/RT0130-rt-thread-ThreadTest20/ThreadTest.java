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
        //Try to make a running thread daemon, test SetDaemonLiveThread
        ThreadRunning1 threadRunning1 = new ThreadRunning1();
        ThreadRunning2 threadRunning2 = new ThreadRunning2();
        ThreadRunning1 threadRunning3 = new ThreadRunning1();
        threadRunning1.setDaemon(true);
        threadRunning3.setDaemon(true);
        threadRunning1.start();
        threadRunning2.start();
        threadRunning3.start();
        try {
            Thread.sleep(1000);
        } catch (Exception e) {
        }
        threadRunning2.stopWork = true;
    }
    static class ThreadRunning1 extends Thread {
        public volatile int i = 0;
        volatile boolean stopWork = false;
        ThreadRunning1() {
            super();
        }
        public void run() {
            while (!stopWork) {
                i++;
            }
            System.out.println("ThreadRunning1");
        }
    }
    static class ThreadRunning2 extends Thread {
        public volatile int i = 0;
        volatile boolean stopWork = false;
        ThreadRunning2() {
            super();
        }
        public void run() {
            while (!stopWork) {
                i++;
                i--;
            }
            System.out.println("ThreadRunning2");
        }
    }
}