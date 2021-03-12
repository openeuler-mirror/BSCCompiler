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
        /**
         * Verify that a thread created by a daemon thread is daemon
        */

        ThreadRunningAnotherThread t = new ThreadRunningAnotherThread();
        t.setDaemon(true);
        t.start();
        t.stop = true;
        try {
            t.join();
        } catch (InterruptedException e) {
            System.out.println("INTERRUPTED_MESSAGE --");
        }
        System.out.println("the child thread of a daemon thread is daemon --- " +
                t.childIsDaemon);
        /**
         * Verify that a thread created by a non-daemon thread is not daemon
        */

        ThreadRunningAnotherThread tt = new ThreadRunningAnotherThread();
        tt.setDaemon(false);
        tt.start();
        tt.stop = true;
        try {
            tt.join();
        } catch (InterruptedException e) {
            System.out.println("INTERRUPTED_MESSAGE --");
        }
        System.out.println("the child thread of a non-daemon thread is non-daemon --- " +
                tt.childIsDaemon);
    }
    // Test for setDaemon() and isDaemon()
    private static class ThreadRunningAnotherThread extends Thread {
        int field = 0;
        volatile boolean stop = false;
        boolean childIsDaemon = false;
        Thread curThread = null;
        public ThreadRunningAnotherThread() {
            super();
        }
        public void run() {
            Thread child = new Thread();
            curThread = Thread.currentThread();
            childIsDaemon = child.isDaemon();
            while (!stop) {
                field++;
            }
        }
    }
}