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


class ThreadTest {
    private static final long waitDuration = 3000;
    private static long waitTime = 0;
    public static void main(String[] args) {
        boolean expired = false;
        ThreadRunning threadRunning = new ThreadRunning();
        threadRunning.start();
        waitTime = waitDuration;
        if (expired) {
            System.out.println("thread' run() method has not started");
        }
        threadRunning.stopWork = true;
        try {
            threadRunning.join();
        } catch (InterruptedException e) {
            System.out.println("INTERRUPTED_MESSAGE");
        }
        threadRunning.interrupt();
        System.out.println("interrupt status has not changed to true -- " + threadRunning.isInterrupted());
        System.out.println("PASS");
    }
    //test for interrupt() -- Interrupt a terminated thread
    /* If this thread is blocked in an invocation
     * of the wait(), wait(long), or wait(long, int) methods of the Object class, or
     * of the join(), join(long), join(long, int), sleep(long), or sleep(long, int), methods of this class,
     * then its interrupt status will be cleared and it will receive an InterruptedException.
    */

    static class ThreadRunning extends Thread {
        public volatile int i = 0;
        volatile boolean stopWork = false;
        ThreadRunning() {
            super();
        }
        public void run() {
            while (!stopWork) {
                i++;
            }
        }
    }
}