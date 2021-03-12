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
    private static boolean doSleep(int interval) {
        try {
            Thread.sleep(interval);
        } catch (InterruptedException e) {
            System.out.println("unexpected InterruptedException while sleeping");
        }
        waitTime -= interval;
        return waitTime <= 0;
    }
    // test for interrupt() -- Interrupt a newly created thread
    // Interrupting a thread that is not alive need not have any effect.
    /* If this thread is blocked in an invocation
     * of the wait(), wait(long), or wait(long, int) methods of the Object class, or
     * of the join(), join(long), join(long, int), sleep(long), or sleep(long, int), methods of this class,
     * then its interrupt status will be cleared and it will receive an InterruptedException.
    */

    public static void main(String[] args) {
        boolean expired = false;
        boolean result = false;
        Thread thread = new Thread();
        thread.interrupt();
        waitTime = waitDuration;
        while (!result && !expired) {
            expired = doSleep(600);
            result = thread.isInterrupted();
        }
        System.out.println("thread's state is -- " + thread.getState());
        System.out.println("isInterrupted() finally returns -- " + result);
        if (expired) {
            System.out.println("interrupt status has not changed to true");
        } else {
            System.out.println("*PASS*");
        }
    }
}