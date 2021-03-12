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
        ThreadRunning threadRunning = new ThreadRunning();
        threadRunning.start();
        System.out.println(threadRunning.isAlive());
        threadRunning.stopWork = true;
        threadRunning.join();
        System.out.println(threadRunning.isAlive());
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