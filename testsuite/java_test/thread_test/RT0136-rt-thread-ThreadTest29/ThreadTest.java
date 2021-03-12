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
    public static void main(String[] args) {
        ThreadJoining t = new ThreadJoining(10000, 0);
        t.start();
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            System.out.println("main throws InterruptedException");
        }
        t.interrupt();
        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) {
            System.out.println("main throws InterruptedException");
        }
        System.out.println("interrupt status has been cleared, if the output is false -- " + t.isInterrupted());
    }
    /* Test for interrupt() -- Interrupt a joining thread
     * If this thread is blocked in an invocation
     * of the wait(), wait(long), or wait(long, int) methods of the Object class, or
     * of the join(), join(long), join(long, int), sleep(long), or sleep(long, int), methods of this class,
     * then its interrupt status will be cleared and it will receive an InterruptedException.
    */

    private static class ThreadJoining extends Thread {
        private long millis;
        private int nanos;
        ThreadJoining(long millis, int nanos) {
            this.millis = millis;
            this.nanos = nanos;
        }
        public void run() {
            try {
                this.join(millis, nanos);
                System.out.println("Fail -- joining thread has not received the InterruptedException");
            } catch (InterruptedException e) {
                System.out.println("joining thread has received the InterruptedException");
            }
        }
    }
}